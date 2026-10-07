#include <H1/Ints.h>

#include <PLATFORM/File.h>

#include <BASE/audio.h>
#include <BASE/backdropWidget.h>
#include <BASE/baseManager.h>
#include <BASE/bitmap.h>
#include <BASE/BITS.h>
#include <BASE/bmap2.h>
#include <BASE/executive.h>
#include <BASE/font.h>
#include <BASE/heroWindow.h>
#include <BASE/heroWindowManager.h>
#include <BASE/icon.h>
#include <BASE/Icon2b.h>
#include <BASE/Icond2b.h>
#include <BASE/Iconm2b.h>
#include <BASE/iconWidget.h>
#include <BASE/inputManager.h>
#include <BASE/message.h>
#include <BASE/Misc.h>
#include <BASE/miscwin.h>
#include <BASE/mouseManager.h>
#include <BASE/resourceManager.h>
#include <BASE/sample.h>
#include <BASE/soundmgr.h>
#include <BASE/textWidget.h>
#include <BASE/TILE.h>
#include <BASE/tileset.h>
#include <BASE/widget.h>
#include <SOURCE/advManager.h>
#include <SOURCE/appMenu.h>
#include <SOURCE/armyGroup.h>
#include <SOURCE/campaignTypes.h>
#include <SOURCE/combatManager.h>
#include <SOURCE/EVENTS.h>
#include <SOURCE/fileRequester.h>
#include <SOURCE/game.h>
#include <SOURCE/hero.h>
#include <SOURCE/highScoreManager.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/mapCell.h>
#include <SOURCE/mapObjectTypes.h>
#include <SOURCE/NOOPT.h>
#include <SOURCE/philAI.h>
#include <SOURCE/playerData.h>
#include <SOURCE/REMOTE.h>
#include <SOURCE/remoteRecords.h>
#include <SOURCE/searchArray.h>
#include <SOURCE/town.h>
#include <SOURCE/wingraph.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

i32 s_drawCloudFrame;
u16 s_drawGroundTile;
b8 s_drawFlipCloud;
u8 s_drawTileset;
b32 s_drawCovered;
i32 s_drawStoneTile;

advManager::advManager(void) {
    i32 i;

    m_groundTiles = NULL;
    m_radarIcon = NULL;
    m_mapOriginX = 0;
    m_mapOriginY = 0;
    m_scrollOffsetX = 0;
    m_scrollOffsetY = 0;
    m_animationFrame = 0;
    m_flagFrameCounter = 0;
    m_pendingCommand = ADVMGR_COMMAND_NONE;
    m_cursorActive = false;
    m_identifyHeroActive = false;
    m_drawHeroShadows = true;
    m_adventureBorder = NULL;
    for (i = 0; i < ADVMGR_OBJECT_ICON_COUNT; i++)
        m_objectIcons[i] = NULL;
    for (i = 0; i < ADVMGR_HERO_ICON_COUNT; i++)
        m_heroIcons[i] = NULL;
    for (i = 0; i < ADVMGR_PLAYER_COLOR_COUNT; i++) {
        m_flagIcons[i] = NULL;
        m_boatFlagIcons[i] = NULL;
    }
    for (i = 0; i < ADVMGR_ENVIRONMENT_SOUND_COUNT; i++)
        m_loopingSamples[i] = NULL;
    for (i = 0; i < ADVMGR_CURSOR_SAMPLE_COUNT; i++)
        m_cursorSamples[i] = NULL;
    m_radarIcon = NULL;
    m_cloudOverlayIcon = NULL;
    m_shadowIcon = NULL;
    m_groundTiles = NULL;
    m_cloudTiles = NULL;
    m_stoneTiles = NULL;
    m_adventureWindow = NULL;
    m_routeMap = NULL;
    m_heroContextLocked = false;
    m_townContextLocked = false;
    gShowIt = true;
    m_combatMonsterX = COMBAT_MONSTER_CELL_NONE;
    m_combatMonsterY = COMBAT_MONSTER_CELL_NONE;
    m_animationPhases[ANIMATION_PHASE_COLUMN_0] = ANIMATION_PHASE_COLUMN_0_INITIAL;
    m_animationPhases[ANIMATION_PHASE_COLUMN_1] = ANIMATION_PHASE_COLUMN_1_INITIAL;
    m_animationPhases[ANIMATION_PHASE_COLUMN_2] = ANIMATION_PHASE_COLUMN_2_INITIAL;
    m_animationPhases[ANIMATION_PHASE_COLUMN_3] = ANIMATION_PHASE_COLUMN_3_INITIAL;
    m_mapData = gGame->GetWorldMapData();
    gMapX = 0;
    gMapY = 0;
    m_cursorFrameCount = 0;
    m_cursorCycle = CURSOR_CYCLE_STOPPED;
    m_cursorTurning = 0;
    // Compared before it is first set (SetHeroContext); none yet, so the
    // first hero's terrain starts its music.
    m_currentTerrain = TERRAIN_INVALID;
}

advManager::~advManager() {}

i16 advManager::Open(i16 id) {
    b32 savedShowIt;
    b32 firstTime;
    i32 oldPlayerVal;
    i32 oldVolume;
    i32 i;

    firstTime = true;
    gCurBottomView = BOTTOM_VIEW_NONE;
    m_heroesLogoShown = false;
    gShowIt = false;
    m_adventureBorder = NULL;
    for (i = 0; i < ADVMGR_BOTTOM_VIEW_WIDGET_COUNT; i++) {
        m_bottomViewPrimaryWidgets[i] = NULL;
        m_bottomViewSecondaryWidgets[i] = NULL;
    }
    if (m_adventureWindow == NULL) {
        m_adventureWindow = new heroWindow(0, 0, "adv_wind.bin");
        if (m_adventureWindow == NULL)
            MemError();
        m_scrollLeftButton = new iconWidget(
            SCROLL_LEFT_X,
            SCROLL_Y,
            SCROLL_WIDTH,
            SCROLL_HEIGHT,
            "scroll.icn",
            SCROLL_ICON_FRAME,
            ICON_DRAW_NORMAL,
            ADVENTURE_CONTROL_HERO_KNOB,
            ICON_WIDGET_DRAW,
            1
        );
        if (m_scrollLeftButton == NULL)
            MemError();
        m_adventureWindow->AddWidget(m_scrollLeftButton, WINDOW_Z_ORDER_APPEND);
        m_scrollRightButton = new iconWidget(
            SCROLL_RIGHT_X,
            SCROLL_Y,
            SCROLL_WIDTH,
            SCROLL_HEIGHT,
            "scroll.icn",
            SCROLL_ICON_FRAME,
            ICON_DRAW_NORMAL,
            ADVENTURE_CONTROL_TOWN_KNOB,
            ICON_WIDGET_DRAW,
            1
        );
        if (m_scrollRightButton == NULL)
            MemError();
        m_adventureWindow->AddWidget(m_scrollRightButton, WINDOW_Z_ORDER_APPEND);
        m_panelBackdrops[0] = new backdropWidget(480, 176, 56, 128, WIDGET_ID_NONE, 32);
        if (m_panelBackdrops[0] == NULL)
            MemError();
        m_panelBackdrops[1] = new backdropWidget(552, 176, 56, 128, WIDGET_ID_NONE, 32);
        if (m_panelBackdrops[1] == NULL)
            MemError();
        m_panelBackdrops[2] = new backdropWidget(539, 194, 10, 92, WIDGET_ID_NONE, 32);
        if (m_panelBackdrops[2] == NULL)
            MemError();
        m_panelBackdrops[3] = new backdropWidget(611, 194, 10, 92, WIDGET_ID_NONE, 32);
        if (m_panelBackdrops[3] == NULL)
            MemError();
        m_panelBackdrops[4] = new backdropWidget(480, 320, 144, 144, WIDGET_ID_NONE, 32);
        if (m_panelBackdrops[4] == NULL)
            MemError();
        for (i = 0; i < ADVMGR_PANEL_ICON_COUNT; i++)
            m_adventureWindow->AddWidget(m_panelBackdrops[i], WINDOW_Z_ORDER_APPEND);
    }
    if (gThisNetHumanPlayer[gCurPlayer])
        gMouseManager->SetPointer("advmice.mse", ADVENTURE_POINTER_DEFAULT);
    else
        gMouseManager->SetPointer("advmice.mse", ADVENTURE_POINTER_WAIT);
    if (m_routeMap == NULL) {
        m_routeMap = new i8[MAP_CELL_GRID_SIZE * MAP_CELL_GRID_SIZE];
        if (m_routeMap == NULL)
            MemError();
    }
    m_routeShown = false;
    gWindowManager->AddWindow(m_adventureWindow, WINDOW_Z_ORDER_BOTTOM, 1);
    if (m_groundTiles == NULL)
        m_groundTiles = gResourceManager->GetTileset("ground32.til");
    if (m_cloudTiles == NULL)
        m_cloudTiles = gResourceManager->GetTileset("clof32.til");
    if (m_stoneTiles == NULL)
        m_stoneTiles = gResourceManager->GetTileset("ston.til");
    if (m_cloudOverlayIcon == NULL)
        m_cloudOverlayIcon = gResourceManager->GetIcon("clop32.icn");
    if (m_objectIcons[TILESET_OBJ32_00] == NULL)
        m_objectIcons[TILESET_OBJ32_00] = gResourceManager->GetIcon("obj32-00.icn");
    if (m_objectIcons[TILESET_OBJ32_01] == NULL)
        m_objectIcons[TILESET_OBJ32_01] = gResourceManager->GetIcon("obj32-01.icn");
    if (m_objectIcons[TILESET_OBJ32_02] == NULL)
        m_objectIcons[TILESET_OBJ32_02] = gResourceManager->GetIcon("obj32-02.icn");
    if (m_objectIcons[TILESET_OBJ32_03] == NULL)
        m_objectIcons[TILESET_OBJ32_03] = gResourceManager->GetIcon("obj32-03.icn");
    if (m_objectIcons[TILESET_OBJ32_04] == NULL)
        m_objectIcons[TILESET_OBJ32_04] = gResourceManager->GetIcon("obj32-04.icn");
    if (m_objectIcons[TILESET_OBJ32_05] == NULL)
        m_objectIcons[TILESET_OBJ32_05] = gResourceManager->GetIcon("obj32-05.icn");
    if (m_objectIcons[TILESET_OBJ32_06] == NULL)
        m_objectIcons[TILESET_OBJ32_06] = gResourceManager->GetIcon("obj32-06.icn");
    if (m_objectIcons[TILESET_OBJ32_07] == NULL)
        m_objectIcons[TILESET_OBJ32_07] = gResourceManager->GetIcon("obj32-07.icn");
    if (m_objectIcons[TILESET_MTN32] == NULL)
        m_objectIcons[TILESET_MTN32] = gResourceManager->GetIcon("mtn32.icn");
    if (m_objectIcons[TILESET_TREE32] == NULL)
        m_objectIcons[TILESET_TREE32] = gResourceManager->GetIcon("tree32.icn");
    if (m_objectIcons[TILESET_TOWN32] == NULL)
        m_objectIcons[TILESET_TOWN32] = gResourceManager->GetIcon("town32.icn");
    if (m_objectIcons[TILESET_RSRC32] == NULL)
        m_objectIcons[TILESET_RSRC32] = gResourceManager->GetIcon("rsrc32.icn");
    if (m_objectIcons[TILESET_MONS32] == NULL)
        m_objectIcons[TILESET_MONS32] = gResourceManager->GetIcon("mons32.icn");
    if (m_objectIcons[TILESET_ART32] == NULL)
        m_objectIcons[TILESET_ART32] = gResourceManager->GetIcon("art32.icn");
    if (m_objectIcons[TILESET_FLAG32] == NULL)
        m_objectIcons[TILESET_FLAG32] = gResourceManager->GetIcon("flag32.icn");
    if (m_objectIcons[TILESET_RESSMALL] == NULL)
        m_objectIcons[TILESET_RESSMALL] = gResourceManager->GetIcon("ressmall.icn");
    if (m_objectIcons[TILESET_HOURGLAS] == NULL)
        m_objectIcons[TILESET_HOURGLAS] = gResourceManager->GetIcon("hourglas.icn");
    if (m_objectIcons[TILESET_ROUTE] == NULL)
        m_objectIcons[TILESET_ROUTE] = gResourceManager->GetIcon("route.icn");
    if (m_objectIcons[TILESET_SMCREST] == NULL)
        m_objectIcons[TILESET_SMCREST] = gResourceManager->GetIcon("smcrest.icn");
    if (m_objectIcons[TILESET_STONBACK] == NULL)
        m_objectIcons[TILESET_STONBACK] = gResourceManager->GetIcon("stonback.icn");
    if (m_objectIcons[TILESET_MINIMON] == NULL)
        m_objectIcons[TILESET_MINIMON] = gResourceManager->GetIcon("minimon.icn");
    if (m_heroIcons[ADVMGR_HERO_ICON_KNIGHT] == NULL)
        m_heroIcons[ADVMGR_HERO_ICON_KNIGHT] = gResourceManager->GetIcon("kngt32.icn");
    if (m_heroIcons[ADVMGR_HERO_ICON_BARBARIAN] == NULL)
        m_heroIcons[ADVMGR_HERO_ICON_BARBARIAN] = gResourceManager->GetIcon("barb32.icn");
    if (m_heroIcons[ADVMGR_HERO_ICON_SORCERESS] == NULL)
        m_heroIcons[ADVMGR_HERO_ICON_SORCERESS] = gResourceManager->GetIcon("sorc32.icn");
    if (m_heroIcons[ADVMGR_HERO_ICON_WARLOCK] == NULL)
        m_heroIcons[ADVMGR_HERO_ICON_WARLOCK] = gResourceManager->GetIcon("wrlk32.icn");
    if (m_heroIcons[ADVMGR_HERO_ICON_BOAT] == NULL)
        m_heroIcons[ADVMGR_HERO_ICON_BOAT] = gResourceManager->GetIcon("boat32.icn");
    gLoadingMonoIcon = true;
    if (m_shadowIcon == NULL)
        m_shadowIcon = gResourceManager->GetIcon("shadow32.icn");
    gLoadingMonoIcon = false;
    if (m_flagIcons[PLAYER_COLOR_BLUE] == NULL)
        m_flagIcons[PLAYER_COLOR_BLUE] = gResourceManager->GetIcon("b-flag32.icn");
    if (m_flagIcons[PLAYER_COLOR_GREEN] == NULL)
        m_flagIcons[PLAYER_COLOR_GREEN] = gResourceManager->GetIcon("g-flag32.icn");
    if (m_flagIcons[PLAYER_COLOR_RED] == NULL)
        m_flagIcons[PLAYER_COLOR_RED] = gResourceManager->GetIcon("r-flag32.icn");
    if (m_flagIcons[PLAYER_COLOR_YELLOW] == NULL)
        m_flagIcons[PLAYER_COLOR_YELLOW] = gResourceManager->GetIcon("y-flag32.icn");
    if (m_boatFlagIcons[PLAYER_COLOR_BLUE] == NULL)
        m_boatFlagIcons[PLAYER_COLOR_BLUE] = gResourceManager->GetIcon("b-bflg32.icn");
    if (m_boatFlagIcons[PLAYER_COLOR_GREEN] == NULL)
        m_boatFlagIcons[PLAYER_COLOR_GREEN] = gResourceManager->GetIcon("g-bflg32.icn");
    if (m_boatFlagIcons[PLAYER_COLOR_RED] == NULL)
        m_boatFlagIcons[PLAYER_COLOR_RED] = gResourceManager->GetIcon("r-bflg32.icn");
    if (m_boatFlagIcons[PLAYER_COLOR_YELLOW] == NULL)
        m_boatFlagIcons[PLAYER_COLOR_YELLOW] = gResourceManager->GetIcon("y-bflg32.icn");
    gLoadingMonoIcon = true;
    if (m_radarIcon == NULL)
        m_radarIcon = gResourceManager->GetIcon("radar.icn");
    gLoadingMonoIcon = false;
    for (i = 0; i < ADVMGR_ENVIRONMENT_SOUND_COUNT; i++)
        m_loopingSamples[i] = NULL;
    for (i = 0; i < ADVMGR_ACTIVE_SOUND_COUNT; i++) {
        m_activeSounds[i].soundId = MAP_SOUND_NONE;
        m_activeSounds[i].distance = ENVIRONMENT_SOUND_FAR_DISTANCE;
        m_activeSoundMask = 0;
    }
    GetCursorSampleSet(gConfig.walkSpeed);
    if (!gThisNetHumanPlayer[gCurPlayer]) {
        gGame->TurnOnAIMusic();
        SetNoDialogMenus(0);
    } else {
        SetNoDialogMenus(1);
    }
    gTimers[ADVENTURE_FRAME_TIMER_SLOT] = KBTickCount() + TIMER_DELAY;
    m_messageTypeMask = MESSAGE_KEY_DOWN | MESSAGE_KEY_UP | MESSAGE_MOUSE_MOVE
                        | MESSAGE_LEFT_BUTTON_DOWN | MESSAGE_RIGHT_BUTTON_DOWN | 0x100
                        | MESSAGE_WIDGET;
    gMouseManager->NewUpdate(true);
    oldVolume = gConfig.soundVolume;
    if (gConfig.soundVolume != SOUND_VOLUME_OFF)
        gConfig.soundVolume = SOUND_VOLUME_LAST;
    SetInitialMapOrigin();
    gShowIt = gThisNetHumanPlayer[gCurPlayer];
    gMouseManager->SetColorMice(false);
    oldPlayerVal = gCurPlayer;
    savedShowIt = gShowIt;
    gCurPlayer = gCurWatchPlayer;
    gCurPlayerData = &gGame->m_players[gCurPlayer];
    gShowIt = true;
    RedrawAdvScreen(true);
    gCurPlayer = oldPlayerVal;
    gShowIt = savedShowIt;
    gCurPlayerData = &gGame->m_players[gCurPlayer];
    if (!gThisNetHumanPlayer[gCurPlayer])
        gGame->ShowComputerScreen();
    gMouseManager->ReallyShowPointer();
    KBChangeMenu(gAdventureMenu);
    gWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_SHORT, gPalette);
    gBottomViewOverride = BOTTOM_VIEW_NONE;
    gConfig.soundVolume = oldVolume;
    SetVolumes(gConfig.soundVolume, gConfig.musicVolume);
    m_messageMask = BASE_MANAGER_ACCEPT_ADVENTURE;
    m_priority = id;
    m_active = 1;
    strcpy(m_name, "advManager");
    return BASE_MANAGER_SUCCESS;
}

void advManager::Close(void) {
    i16 index;

    ClearBottomView();
    gMouseManager->SetPointer(MOUSE_INVALID_CURSOR_FRAME);
    StopAllAudio();
    if (m_adventureBorder) {
        free(m_adventureBorder);
        m_adventureBorder = NULL;
    }
    if (gAdvDisposeLevel <= ADV_DISPOSE_OBJECT_ICONS_LAST) {
        for (index = 0; index < ADVMGR_OBJECT_ICON_COUNT; index++) {
            if (m_objectIcons[index])
                gResourceManager->Dispose(m_objectIcons[index]);
            m_objectIcons[index] = NULL;
        }
    }
    if (gAdvDisposeLevel <= ADV_DISPOSE_NONE) {
        gResourceManager->Dispose(m_radarIcon);
        m_radarIcon = NULL;
        gResourceManager->Dispose(m_cloudOverlayIcon);
        m_cloudOverlayIcon = NULL;
        for (index = 0; index < ADVMGR_HERO_ICON_COUNT; index++) {
            gResourceManager->Dispose(m_heroIcons[index]);
            m_heroIcons[index] = NULL;
        }
        gResourceManager->Dispose(m_shadowIcon);
        m_shadowIcon = NULL;
        for (index = 0; index < ADVMGR_PLAYER_COLOR_COUNT; index++) {
            gResourceManager->Dispose(m_flagIcons[index]);
            m_flagIcons[index] = NULL;
            gResourceManager->Dispose(m_boatFlagIcons[index]);
            m_boatFlagIcons[index] = NULL;
        }
        gResourceManager->Dispose(m_groundTiles);
        m_groundTiles = NULL;
        gResourceManager->Dispose(m_cloudTiles);
        m_cloudTiles = NULL;
        gResourceManager->Dispose(m_stoneTiles);
        m_stoneTiles = NULL;
    }
    for (index = 0; index < ADVMGR_ENVIRONMENT_SOUND_COUNT; index++) {
        if (m_loopingSamples[index])
            gResourceManager->Dispose(m_loopingSamples[index]);
        m_loopingSamples[index] = NULL;
    }
    for (index = 0; index < ADVMGR_CURSOR_SAMPLE_COUNT; index++) {
        if (m_cursorSamples[index])
            gResourceManager->Dispose(m_cursorSamples[index]);
        m_cursorSamples[index] = NULL;
    }
    gWindowManager->RemoveWindow(m_adventureWindow);
    delete m_adventureWindow;
    m_adventureWindow = NULL;
    if (m_routeMap)
        delete[] m_routeMap;
    m_routeMap = NULL;
    gCurBottomView = BOTTOM_VIEW_NONE;
    m_active = 0;
}

void advManager::GetCursorSampleSet(i32 sampleSet) {
    if (sampleSet >= 1)
        sampleSet = CURSOR_SAMPLE_FAST_SET;
    i8 suffixSample[ADVMGR_CURSOR_SAMPLE_COUNT] =
        {0, 3, 5, 3, 4, 5, 6};
    for (i32 index = TERRAIN_WATER; index < TERRAIN_COUNT; ++index) {
        sprintf(gText, "wsnd%1d%1d.82M", sampleSet, suffixSample[index]);
        m_cursorSamples[index] = gResourceManager->GetSample(gText);
        m_cursorSamples[index]->m_playbackData.volume = SAMPLE_VOLUME_FULL;
    }
}

class mapCell* advManager::DoAdvCommand(void) {
    i16 pathIndex;
    b8 moveDone;
    b32 anyMoveChanged;
    b8 hover;
    tag_message evt;
    mapCell* cellPtr;
    b32 oldMapValid;
    hero* selectedHero;
    town* viewTown;
    b8 userStopRequested;

    cellPtr = NULL;
    // The commands that do not move a hero (town view, ...) also run with no
    // current hero (-1); the original took the address of the hero before
    // the table for them, unused.
    selectedHero = gCurPlayerData->m_currentHero != HERO_ID_NONE
                       ? gGame->GetHero(gCurPlayerData->m_currentHero)
                       : NULL;
    // A move or occupied-town command set while a hero was selected can
    // outlive the selection (the hover that set it is not repeated); the
    // original then moved hero -1. It is dropped.
    if (selectedHero == NULL
        && (m_pendingCommand == ADVMGR_COMMAND_MOVE_TO
            || m_pendingCommand == ADVMGR_COMMAND_CONTINUE_ROUTE
            || m_pendingCommand == ADVMGR_COMMAND_OCCUPIED_TOWN_VIEW))
        m_pendingCommand = ADVMGR_COMMAND_NONE;
    userStopRequested = false;
    hover = false;
    switch (m_pendingCommand) {
        case ADVMGR_COMMAND_MOVE_TO:
            selectedHero->m_destinationX = m_commandTargetX;
            selectedHero->m_destinationY = m_commandTargetY;
            goto continue_route;
        case ADVMGR_COMMAND_CONTINUE_ROUTE:
        continue_route:
            gSearchArray->BuildPath(
                selectedHero->m_x,
                selectedHero->m_y,
                selectedHero->m_destinationX,
                selectedHero->m_destinationY,
                SEARCH_UNLIMITED_COST
            );
            if (gSearchArray->m_pathLength > 0) {
                oldMapValid = m_routeShown;
                MobilizeCurrHero(true);
                if (gConfig.showRoute || oldMapValid)
                    ShowRoute(false, 0, false);
                else if (m_routeShown && m_pendingCommand != ADVMGR_COMMAND_CONTINUE_ROUTE)
                    HideRoute(true, false, true);
                gMouseManager->ReallyHidePointer();
                gInputManager->Flush();
                for (pathIndex = gSearchArray->m_pathLength - 1; pathIndex >= 0; pathIndex--) {
                    cellPtr = MoveHero(
                        gSearchArray->m_directions[pathIndex],
                        pathIndex == 0,
                        &gTriggerX,
                        &gTriggerY,
                        &anyMoveChanged,
                        false,
                        &moveDone
                    );
                    UpdateHeroLocator(LOCATOR_SLOT_CURRENT_HERO, true, true);
                    if (cellPtr)
                        break;
                    if (anyMoveChanged || moveDone)
                        goto movement_done;
                    evt = gInputManager->GetEvent();
                    while (evt.type) {
                        if (evt.type == MESSAGE_KEY_DOWN || evt.type == MESSAGE_LEFT_BUTTON_DOWN
                            || evt.type == MESSAGE_RIGHT_BUTTON_DOWN
                            || evt.type == MESSAGE_WIDGET) {
                            userStopRequested = true;
                            StopCursor(true);
                            goto movement_done;
                        }
                        Process1WindowsMessage();
                        evt = gInputManager->GetEvent();
                    }
                }
            movement_done:
                if ((pathIndex <= 0 && selectedHero->m_x == selectedHero->m_destinationX
                     && selectedHero->m_y == selectedHero->m_destinationY)
                    || (userStopRequested && !gConfig.showRoute) || cellPtr)
                    HideRoute(true, true, true);
                else if (m_pendingCommand == ADVMGR_COMMAND_CONTINUE_ROUTE || gConfig.showRoute)
                    ShowRoute(false, 1, true);
                gMouseManager->ReallyShowPointer();
                UpdBottomView(true, true, true);
                if (cellPtr) {
                    StopCursor(true);
                    DoEvent(cellPtr, gTriggerX, gTriggerY);
                    cellPtr = NULL;
                }
                Reseed(0, 0);
                hover = true;
                CheckDimHero();
            }
            break;
        case ADVMGR_COMMAND_OCCUPIED_TOWN_VIEW:
            DemobilizeCurrHero();
            gMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
            viewTown = gGame->GetTown(selectedHero->m_occupiedTown);
            viewTown->View();
            cellPtr = NULL;
            break;
        case ADVMGR_COMMAND_TOWN_VIEW:
            DemobilizeCurrHero();
            gMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
            cellPtr = GetCell(
                gGame->GetTown(gCurPlayerData->m_currentTown)->m_x,
                gGame->GetTown(gCurPlayerData->m_currentTown)->m_y
            );
            gGame->GetTown(gCurPlayerData->m_currentTown)->View();
            cellPtr = NULL;
            break;
        case ADVMGR_COMMAND_HERO_VIEW:
            gMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
            gGame->GetHero(gCurPlayerData->m_currentHero)->HeroView(false);
            RedrawAdvScreen(true);
            gWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_SHORT, NULL);
            break;
        case ADVMGR_COMMAND_SELECT_HERO:
            SetHeroContext(
                GetCell(m_mapOriginX + m_hoverCellX, m_mapOriginY + m_hoverCellY)->m_objectMetadata,
                false
            );
            break;
        case ADVMGR_COMMAND_SELECT_TOWN:
            SetTownContext(
                GetCell(m_mapOriginX + m_hoverCellX, m_mapOriginY + m_hoverCellY)->m_objectMetadata
            );
            break;
        case ADVMGR_COMMAND_NONE:
            break;
    }
    m_pendingCommand = ADVMGR_COMMAND_NONE;
    m_hoverCellX = m_hoverCellY = CURSOR_INVALID_POSITION;
    if (hover)
        ForceNewHover();
    return cellPtr;
}

i32 gLastScrollTime = 0;
i32 gSandAnim = 0;
i32 gLastHourGlassUpdateTime = 0;
i32 gTriggerX = 0;
i32 gTriggerY = 0;
i32 gCurBottomView = BOTTOM_VIEW_NONE;
i32 gCurHourGlassPhase = 0;
b32 gForceUpdate = false;
i32 gUnusedAdvCount = 0;

b8 gCheatUnlimitedMovement[GAME_PLAYER_COUNT][GAME_HERO_COUNT];
b8 gCheatUnlimitedSpells[GAME_PLAYER_COUNT][GAME_HERO_COUNT];

void ClearCheatState(void) {
    memset(gCheatUnlimitedMovement, 0, sizeof(gCheatUnlimitedMovement));
    memset(gCheatUnlimitedSpells, 0, sizeof(gCheatUnlimitedSpells));
}

// The extended cheat codes act on the current player, its current hero and
// its current town.
void advManager::ApplyExtendedCheat(i32 code) {
    hero* curHero;
    town* curTown;
    i32 i;
    i32 level;
    i32 experience;
    i8 creature;
    i8 artifact;
    i16 growth;
    i32 townType;

    curHero = NULL;
    if (gCurPlayerData->m_currentHero != HERO_ID_NONE)
        curHero = gGame->GetHero(gCurPlayerData->m_currentHero);
    if (code == CHEAT_RESOURCES || code == CHEAT_ALL) {
        for (i = 0; i < RESOURCE_COUNT; i++) {
            if (i == RESOURCE_GOLD)
                gCurPlayerData->m_resources[i] += CHEAT_GOLD_GIFT;
            else if (i == RESOURCE_WOOD || i == RESOURCE_ORE)
                gCurPlayerData->m_resources[i] += CHEAT_COMMON_RESOURCE_GIFT;
            else
                gCurPlayerData->m_resources[i] += CHEAT_RARE_RESOURCE_GIFT;
        }
        RedrawAdvScreen(true);
    }
    if (code == CHEAT_CASTLE && gCurPlayerData->m_currentTown != GAME_TOWN_NONE) {
        curTown = gGame->GetTown(gCurPlayerData->m_currentTown);
        if (curTown->m_buildings & (1 << BUILDING_SLOT_TENT)) {
            curTown->XformToCastle();
            curTown->m_buildings -= (1 << BUILDING_SLOT_TENT);
        }
        curTown->m_buildings |= (1 << BUILDING_SLOT_MAGE_GUILD) | (1 << BUILDING_SLOT_THIEVES_GUILD)
                                | (1 << BUILDING_SLOT_TAVERN) | (1 << BUILDING_SLOT_WELL)
                                | (1 << BUILDING_SLOT_CASTLE);
        for (i = BUILDING_SLOT_DWELLING_FIRST; i <= BUILDING_SLOT_DWELLING_LAST; i++)
            curTown->m_buildings |= 1 << i;
        if (GetCell(curTown->m_x - 1, curTown->m_y + 1)->m_tileIndex < MAP_CELL_TILES_PER_TERRAIN)
            curTown->m_buildings |= (1 << BUILDING_SLOT_SHIPYARD);
        curTown->m_buildState = MAGE_GUILD_STATE_LEVEL_4;
        for (i = 0; i < BUILDING_SLOT_DWELLING_COUNT; i++) {
            growth =
                gMonsterDatabase[gDwellingType[curTown->m_type][i]].growth * CHEAT_CREATURE_WEEKS;
            if (curTown->m_dwellingAvailable[i] > CHEAT_GARRISON_MAX - growth)
                curTown->m_dwellingAvailable[i] = CHEAT_GARRISON_MAX;
            else
                curTown->m_dwellingAvailable[i] += growth;
        }
        SetTownContext(curTown->m_id);
    }
    if (!curHero)
        return;
    if (code == CHEAT_MOVEMENT || code == CHEAT_ALL) {
        gCheatUnlimitedMovement[gCurPlayer][curHero->m_id] = true;
        curHero->m_mobility = curHero->m_remainingMobility = CHEAT_UNLIMITED_MOBILITY;
        SetHeroContext(curHero->m_id, false);
    }
    if (code == CHEAT_MAGIC || code == CHEAT_ALL) {
        if (!curHero->HasArtifact(ARTIFACT_MAGIC_BOOK))
            GiveArtifact(curHero, ARTIFACT_MAGIC_BOOK);
        for (i = 0; i < HERO_SPELL_SLOT_COUNT; i++)
            curHero->AddSpell(i, CHEAT_SPELL_CHARGES_FULL, false);
        gCheatUnlimitedSpells[gCurPlayer][curHero->m_id] = true;
    }
    if (code >= CHEAT_LEVELS_FIRST && code <= CHEAT_LEVELS_LAST) {
        level = curHero->GetLevel(curHero->m_experience) + code - CHEAT_ALL;
        if (level > CHEAT_MAX_LEVEL)
            level = CHEAT_MAX_LEVEL;
        // Beyond the experience table a level needs more than its
        // threshold.
        experience = curHero->GetExperience(level);
        if (level > HERO_EXPERIENCE_LEVEL_TABLE_COUNT)
            experience++;
        if (experience > curHero->m_experience)
            GiveExperience(curHero, experience - curHero->m_experience, true);
    }
    if ((code >= CHEAT_CREATURE_FIRST && code <= CHEAT_CREATURE_LAST) || code == CHEAT_ALL) {
        if (code == CHEAT_ALL) {
            // The strongest creature of the hero's own town.
            for (townType = 0; townType < TOWN_TYPE_COUNT; townType++) {
                if (gTownHeroClass[townType] == curHero->m_heroClass)
                    break;
            }
            creature = gDwellingType[townType][BUILDING_SLOT_DWELLING_COUNT - 1];
        } else {
            creature = code % CHEAT_CODE_ITEM_MODULUS;
        }
        if (curHero->m_army.CanJoin(creature))
            curHero->m_army.Add(
                creature,
                gMonsterDatabase[creature].growth * CHEAT_CREATURE_WEEKS,
                ARMY_GROUP_EMPTY_SLOT
            );
        RedrawAdvScreen(true);
    }
    if (code >= CHEAT_ARTIFACT_FIRST && code <= CHEAT_ARTIFACT_LAST) {
        artifact = code % CHEAT_CODE_ITEM_MODULUS;
        if (artifact != ARTIFACT_MAGIC_BOOK || !curHero->HasArtifact(ARTIFACT_MAGIC_BOOK))
            GiveArtifact(curHero, artifact);
    }
}

// F5 saves the game under a fixed name, numbered like the autosave.
void advManager::QuickSave(void) {
    gGame->SaveGame("QUICKSAVE", true);
    NormalDialog(
        localization::Tr("adventure.quick_save.done"),
        NORMAL_DIALOG_TYPE_OK,
        0xb1,
        0x50
    );
}

// F9 asks to reload the quick save; the game is then reloaded from the main
// loop like any loaded game. Not available in network games.
b32 advManager::QuickLoad(void) {
    char path[452];

    if (gRemoteOn)
        return false;
    NormalDialog(
        localization::Tr("adventure.quick_load.confirm"),
        NORMAL_DIALOG_TYPE_YES_NO,
        0xb1,
        0x50
    );
    if (gWindowManager->m_dialogResult != NORMAL_DIALOG_CONFIRM)
        return false;
    gGame->QuickSaveName(gLastFilename);
    sprintf(path, "%s%s", gGamePath, gLastFilename);
    if (!FileExists(path)) {
        NormalDialog(
            localization::Tr("adventure.quick_load.missing"),
            NORMAL_DIALOG_TYPE_OK,
            0xb1,
            0x50
        );
        return false;
    }
    return true;
}

i16 advManager::Main(struct tag_message& message) {
    static i32 gCheatSeq = 0;
    i32 yPos;
    i32 xPos;
    i32 result;
    mapCell* location;
    hero* curHero;
    i32 townIndex;
    i32 amount;
    i32 quit;
    b32 movedSet;
    b8 bEnded;
    i32 newHelpText;
    i32 orient;

    if (gTimers[ADVENTURE_FRAME_TIMER_SLOT] < KBTickCount() && ComboDraw(true))
        UpdateScreen(true, false);
    if (gGameOver) {
        message.type = MESSAGE_EXECUTIVE;
        message.executiveCommand = EXECUTIVE_COMMAND_TERMINATE_LOOP;
        return MESSAGE_DISPATCH_FORWARD;
    }
    if (!gHumanPlayer[gCurPlayer] && (!gRemoteOn || gThisGamePos == gHostGamePos)) {
        gPhilAI->DoAI(gCurPlayer);
        gGame->NextPlayer();
        return MESSAGE_DISPATCH_CONSUME;
    }
    CheckHandleNet();
    if (!gThisNetHumanPlayer[gCurPlayer])
        return CheckHandleNetPlayerWait(message, false);
    if (gScreenScroll && gForegroundApp)
        CheckScreenScroll();
    if (!(message.type & m_messageTypeMask)) {
        if (message.type) {
            message.type = MESSAGE_NONE;
            return MESSAGE_DISPATCH_FORWARD;
        }
        return MESSAGE_DISPATCH_CONTINUE;
    }
    if (!gNoSound && gConfig.musicVolume && gForceSwitchMusic > 0
        && KBTickCount() - gForceSwitchMusic > FORCED_MUSIC_DELAY
        && GetCurrentTrack() == MUSIC_TRACK_NETWORK_TURN) {
        gForceSwitchMusic = FORCED_MUSIC_IDLE;
        PlayMusic(TERRAIN_MUSIC_TRACK(m_currentTerrain));
    }
    result = MESSAGE_DISPATCH_CONSUME;
    quit = 0;
    location = NULL;
    if (message.type) {
        switch (message.type) {
            case MESSAGE_WIDGET:
                switch (message.command) {
                    case WIDGET_COMMAND_HOVER:
                        result = ProcessHover(&message);
                        break;
                    case WIDGET_NOTIFY_DESELECT:
                        if (!(message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON))
                            result = ProcessDeSelect(&message, &quit, &location);
                        break;
                    case WIDGET_NOTIFY_SELECT:
                        result = ProcessSelect(&message, &location);
                        break;
                    case WIDGET_NOTIFY_RIGHT_CLICK:
                        newHelpText = ADVENTURE_HELP_NONE;
                        switch (message.id) {
                            case ADVENTURE_CONTROL_NEXT_HERO:
                                newHelpText = ADVENTURE_HELP_NEXT_HERO;
                                break;
                            case ADVENTURE_CONTROL_CONTINUE_ROUTE:
                                newHelpText = ADVENTURE_HELP_CONTINUE_ROUTE;
                                break;
                            case ADVENTURE_CONTROL_OVERVIEW:
                                newHelpText = ADVENTURE_HELP_OVERVIEW;
                                break;
                            case ADVENTURE_CONTROL_END_TURN:
                                newHelpText = ADVENTURE_HELP_END_TURN;
                                break;
                            case ADVENTURE_CONTROL_ADVENTURE_OPTIONS:
                                newHelpText = ADVENTURE_HELP_ADVENTURE_OPTIONS;
                                break;
                            case ADVENTURE_CONTROL_GAME_OPTIONS:
                                newHelpText = ADVENTURE_HELP_GAME_OPTIONS;
                                break;
                        }
                        if (newHelpText >= ADVENTURE_HELP_FIRST)
                            NormalDialog(gAdvMenuHelp[newHelpText], NORMAL_DIALOG_TYPE_QUICK_VIEW);
                        break;
                }
                break;
            case MESSAGE_KEY_DOWN:
                orient = MAP_DIRECTION_NONE;
                if (gCurPlayerData->CurrentHero() != HERO_ID_NONE)
                    curHero = gGame->GetHero(gCurPlayerData->m_currentHero);
                else
                    curHero = NULL;
                if (gDebugLevel < ADVENTURE_DEBUG_KEYS_LEVEL_MIN
                    && (message.keyCode == INPUT_SCAN_F3 || message.keyCode == INPUT_SCAN_F4
                        || message.keyCode == INPUT_SCAN_F5 || message.keyCode == INPUT_SCAN_F6
                        || message.keyCode == INPUT_SCAN_F7 || message.keyCode == INPUT_SCAN_F8
                        || message.keyCode == INPUT_SCAN_F9 || message.keyCode == INPUT_SCAN_F10
                        || message.keyCode == INPUT_SCAN_F11
                        || message.keyCode == INPUT_SCAN_F12)) {
                    // Outside debug mode F5 saves the game in place and F9
                    // reloads that save.
                    if (message.keyCode == INPUT_SCAN_F5) {
                        QuickSave();
                    } else if (message.keyCode == INPUT_SCAN_F9 && QuickLoad()) {
                        quit = 1;
                        gGameCommand = MAIN_MENU_QUICK_LOAD;
                    }
                    break;
                }
                switch (message.keyCode) {
                    case INPUT_SCAN_F2:
                        PopNetBox(NULL);
                        break;
                    case INPUT_SCAN_F3:
                        gGame->m_playerDead[1] = true;
                        gGame->m_playerDead[2] = true;
                        gGame->m_playerDead[3] = true;
                        CheckEndGame(true);
                        break;
                    case INPUT_SCAN_F5:
                        gGame->m_playerDead[0] = true;
                        CheckEndGame(false);
                        break;
                    case INPUT_SCAN_F6:
                        if (curHero) {
                            for (amount = 0; amount < HERO_SPELL_SLOT_COUNT; amount++)
                                curHero->AddSpell(
                                    amount,
                                    CHEAT_SPELL_CHARGES,
                                    false
                                );
                        }
                        break;
                    case INPUT_SCAN_F7:
                        if (curHero)
                            GiveExperience(curHero, CHEAT_EXPERIENCE_AMOUNT, true);
                        break;
                    case INPUT_SCAN_F8:
                        if (curHero) {
                            gGame->GiveArmy(
                                &curHero->m_army,
                                CREATURE_DRAGON,
                                1,
                                ARMY_GROUP_EMPTY_SLOT
                            );
                            gGame->GiveArmy(
                                &curHero->m_army,
                                CREATURE_TROLL,
                                1,
                                ARMY_GROUP_EMPTY_SLOT
                            );
                        }
                        break;
                    case INPUT_SCAN_F9:
                        for (amount = 0; amount < RESOURCE_COUNT;
                             amount++) {
                            gCurPlayerData->m_resources[amount] +=
                                (amount == RESOURCE_GOLD
                                     ? CHEAT_GOLD_AMOUNT
                                     : CHEAT_RESOURCE_AMOUNT);
                        }
                        break;
                    case INPUT_SCAN_F11:
                        if (curHero)
                            curHero->m_remainingMobility = CHEAT_MOBILITY;
                        break;
                    case INPUT_SCAN_F12:
                        gGame->SetVisibility(
                            CHEAT_REVEAL_CENTER,
                            CHEAT_REVEAL_CENTER,
                            gCurPlayer,
                            CHEAT_REVEAL_RADIUS
                        );
                        UpdateRadar(true, false);
                        CompleteDraw(false);
                        UpdateScreen(false, false);
                        break;
                    case INPUT_SCAN_0:
                        amount = 0;
                        goto processCheatDigit;
                    case INPUT_SCAN_1:
                        amount = 1;
                        goto processCheatDigit;
                    case INPUT_SCAN_2:
                        amount = 2;
                        goto processCheatDigit;
                    case INPUT_SCAN_3:
                        amount = 3;
                        goto processCheatDigit;
                    case INPUT_SCAN_4:
                        amount = 4;
                        goto processCheatDigit;
                    case INPUT_SCAN_5:
                        amount = 5;
                        goto processCheatDigit;
                    case INPUT_SCAN_6:
                        amount = 6;
                        goto processCheatDigit;
                    case INPUT_SCAN_7:
                        amount = 7;
                        goto processCheatDigit;
                    case INPUT_SCAN_8:
                        amount = 8;
                        goto processCheatDigit;
                    case INPUT_SCAN_9:
                        amount = 9;
                        goto processCheatDigit;
                    processCheatDigit:
                        if (gConfig.cheatMode == CHEAT_MODE_OFF)
                            break;
                        gCheatSeq =
                            gCheatSeq * CHEAT_SEQUENCE_RADIX % CHEAT_SEQUENCE_MODULUS + amount;
                        if (gConfig.cheatMode == CHEAT_MODE_EXTENDED)
                            ApplyExtendedCheat(gCheatSeq);
                        if (gCheatSeq == CHEAT_REVEAL_MAP) {
                            gGame->SetVisibility(
                                CHEAT_REVEAL_CENTER,
                                CHEAT_REVEAL_CENTER,
                                0,
                                CHEAT_REVEAL_RADIUS
                            );
                            gGame->SetVisibility(
                                CHEAT_REVEAL_CENTER,
                                CHEAT_REVEAL_CENTER,
                                1,
                                CHEAT_REVEAL_RADIUS
                            );
                            gGame->SetVisibility(
                                CHEAT_REVEAL_CENTER,
                                CHEAT_REVEAL_CENTER,
                                2,
                                CHEAT_REVEAL_RADIUS
                            );
                            gGame->SetVisibility(
                                CHEAT_REVEAL_CENTER,
                                CHEAT_REVEAL_CENTER,
                                3,
                                CHEAT_REVEAL_RADIUS
                            );
                            Reseed(0, 0);
                            UpdateRadar(true, false);
                            CompleteDraw(false);
                            UpdateScreen(false, false);
                        }
                        break;
                    case INPUT_SCAN_ESCAPE:
                        break;
                    case INPUT_SCAN_NUMPAD_8:
                        if (message.modifiers & MESSAGE_MODIFIER_CONTROL_KEYS)
                            ScreenScroll(MAP_DIRECTION_NORTH, false);
                        else
                            orient = MAP_DIRECTION_NORTH;
                        break;
                    case INPUT_SCAN_NUMPAD_9:
                        if (message.modifiers & MESSAGE_MODIFIER_CONTROL_KEYS)
                            ScreenScroll(MAP_DIRECTION_NORTH_EAST, false);
                        else
                            orient = MAP_DIRECTION_NORTH_EAST;
                        break;
                    case INPUT_SCAN_NUMPAD_6:
                        if (message.modifiers & MESSAGE_MODIFIER_CONTROL_KEYS)
                            ScreenScroll(MAP_DIRECTION_EAST, false);
                        else
                            orient = MAP_DIRECTION_EAST;
                        break;
                    case INPUT_SCAN_NUMPAD_3:
                        if (message.modifiers & MESSAGE_MODIFIER_CONTROL_KEYS)
                            ScreenScroll(MAP_DIRECTION_SOUTH_EAST, false);
                        else
                            orient = MAP_DIRECTION_SOUTH_EAST;
                        break;
                    case INPUT_SCAN_NUMPAD_2:
                        if (message.modifiers & MESSAGE_MODIFIER_CONTROL_KEYS)
                            ScreenScroll(MAP_DIRECTION_SOUTH, false);
                        else
                            orient = MAP_DIRECTION_SOUTH;
                        break;
                    case INPUT_SCAN_NUMPAD_1:
                        if (message.modifiers & MESSAGE_MODIFIER_CONTROL_KEYS)
                            ScreenScroll(MAP_DIRECTION_SOUTH_WEST, false);
                        else
                            orient = MAP_DIRECTION_SOUTH_WEST;
                        break;
                    case INPUT_SCAN_NUMPAD_4:
                        if (message.modifiers & MESSAGE_MODIFIER_CONTROL_KEYS)
                            ScreenScroll(MAP_DIRECTION_WEST, false);
                        else
                            orient = MAP_DIRECTION_WEST;
                        break;
                    case INPUT_SCAN_NUMPAD_7:
                        if (message.modifiers & MESSAGE_MODIFIER_CONTROL_KEYS)
                            ScreenScroll(MAP_DIRECTION_NORTH_WEST, false);
                        else
                            orient = MAP_DIRECTION_NORTH_WEST;
                        break;
                    case INPUT_SCAN_C:
                        CheckCastSpell();
                        break;
                    case INPUT_SCAN_D:
                        ProcessSearch(ADVMGR_SEARCH_VIEW_CENTER, ADVMGR_SEARCH_VIEW_CENTER);
                        break;
                    case INPUT_SCAN_P:
                        ViewPuzzle();
                        break;
                    case INPUT_SCAN_V:
                        ViewWorld(SPELL_VIEW_ALL, false, false);
                        break;
                    case INPUT_SCAN_N:
                        amount = MAIN_MENU_NEW_GAME;
                        strcpy(gText, localization::Tr("adventure.confirm_restart"));
                        goto confirmGameCommand;
                    case INPUT_SCAN_L:
                        amount = MAIN_MENU_LOAD_GAME;
                        strcpy(gText, localization::Tr("adventure.confirm_load"));
                        goto confirmGameCommand;
                    case INPUT_SCAN_Q:
                        amount = MAIN_MENU_QUIT;
                        strcpy(gText, localization::Tr("adventure.confirm_quit"));
                        goto confirmGameCommand;
                    confirmGameCommand:
                        quit = 1;
                        NormalDialog(gText, NORMAL_DIALOG_TYPE_YES_NO);
                        if (gWindowManager->m_dialogResult == NORMAL_DIALOG_CANCEL)
                            quit = 0;
                        else
                            gGameCommand = amount;
                        break;
                    case INPUT_SCAN_S:
                        SaveGame();
                        break;
                    case INPUT_SCAN_I:
                        if (gGame->m_campaignType > 0)
                            gGame->ShowCampaignInfo(gGame->m_campaignScenario, true, 0);
                        else
                            gGame->ShowScenInfo();
                        break;
                    case INPUT_SCAN_T:
                        // The original tested >= 0 and, for a player without
                        // towns, selected the stale or empty (-1) first entry.
                        if (gCurPlayerData->m_townCount > 0) {
                            if (gCurPlayerData->CurrentTown() == GAME_TOWN_NONE) {
                                townIndex = gCurPlayerData->m_townIds[0];
                            } else {
                                townIndex = 0;
                                for (amount = 0; amount < gCurPlayerData->m_townCount; amount++) {
                                    if (gCurPlayerData->CurrentTown()
                                        == gCurPlayerData->m_townIds[amount]) {
                                        if (amount == gCurPlayerData->m_townCount - 1)
                                            townIndex = gCurPlayerData->m_townIds[0];
                                        else
                                            townIndex = gCurPlayerData->m_townIds[amount + 1];
                                    }
                                }
                            }
                            SetTownContext(townIndex);
                        }
                        break;
                    case INPUT_SCAN_H:
                        SetHeroContext(gCurPlayerData->NextHero(0), false);
                        break;
                    case INPUT_SCAN_ENTER:
                        if (gCurPlayerData->CurrentTown() != GAME_TOWN_NONE) {
                            m_pendingCommand = ADVMGR_COMMAND_TOWN_VIEW;
                            DoAdvCommand();
                        } else if (gCurPlayerData->CurrentHero() != HERO_ID_NONE) {
                            m_pendingCommand = ADVMGR_COMMAND_HERO_VIEW;
                            DoAdvCommand();
                        }
                        break;
                }
                if (gCurPlayerData->m_currentHero != HERO_ID_NONE
                    && orient >= MAP_DIRECTION_FIRST) {
                    HideRoute(true, true, true);
                    gMouseManager->ReallyHidePointer();
                    location =
                        MoveHero(orient, true, &gTriggerX, &gTriggerY, &movedSet, false, &bEnded);
                    UpdateHeroLocator(LOCATOR_SLOT_CURRENT_HERO, true, true);
                    if (location) {
                        StopCursor(true);
                        DoEvent(location, gTriggerX, gTriggerY);
                        location = NULL;
                    }
                    Reseed(0, 0);
                    ForceNewHover();
                    UpdBottomView(true, true, true);
                    CheckDimHero();
                    gMouseManager->ReallyShowPointer();
                }
                break;
        }
    }
    if (location)
        DoEvent(location, gTriggerX, gTriggerY);
    if (gGameOver || quit == 1 || gMenuCommand != APP_MENU_NONE) {
        message.type = MESSAGE_EXECUTIVE;
        message.executiveCommand = EXECUTIVE_COMMAND_TERMINATE_LOOP;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return result;
}

void advManager::Reseed(i32, i32) {
    gSeedingValid = false;
}

i32 advManager::ProcessSelect(struct tag_message* message, class mapCell** eventCell) {
    i32 iPage;
    i16 mouseX;
    i16 objectTypeState;
    i16 mouseY;
    i16 objectIdIndex;
    b32 visible;
    mapCell* theCell;
    tag_message mouseMsg;
    tag_message inputMessage;
    b8 mobileResult;
    hero* currentHero;
    b32 hasRecord;

    visible = true;
    switch (message->id) {
        case ADVENTURE_CONTROL_HERO_LOCATOR_1:
        case ADVENTURE_CONTROL_HERO_LOCATOR_2:
        case ADVENTURE_CONTROL_HERO_LOCATOR_3:
        case ADVENTURE_CONTROL_HERO_LOCATOR_4:
            iPage = (message->id - ADVENTURE_CONTROL_HERO_LOCATOR_1)
                    / (ADVENTURE_CONTROL_HERO_LOCATOR_2 - ADVENTURE_CONTROL_HERO_LOCATOR_1);
            if (iPage >= gCurPlayerData->m_heroCount)
                break;
            objectTypeState = gCurPlayerData->m_heroIds[gCurPlayerData->m_heroLocatorPage + iPage];
            if (message->modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON) {
                HeroQuickView(objectTypeState, iPage, QUICK_VIEW_AT_LOCATOR, QUICK_VIEW_AT_LOCATOR);
            } else if (objectTypeState == gCurPlayerData->CurrentHero()) {
                m_pendingCommand = ADVMGR_COMMAND_HERO_VIEW;
                DoAdvCommand();
            } else {
                HideRoute(true, false, true);
                SetHeroContext(objectTypeState, false);
            }
            break;
        case ADVENTURE_CONTROL_TOWN_LOCATOR_1:
        case ADVENTURE_CONTROL_TOWN_LOCATOR_2:
        case ADVENTURE_CONTROL_TOWN_LOCATOR_3:
        case ADVENTURE_CONTROL_TOWN_LOCATOR_4:
            objectTypeState = gCurPlayerData->m_townIds
                                  [gCurPlayerData->m_townLocatorPage + message->id
                                   - ADVENTURE_CONTROL_TOWN_LOCATOR_1];
            if (message->modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON) {
                TownQuickView(
                    objectTypeState,
                    message->id - ADVENTURE_CONTROL_TOWN_LOCATOR_1,
                    QUICK_VIEW_AT_LOCATOR,
                    QUICK_VIEW_AT_LOCATOR
                );
            } else {
                HideRoute(true, false, true);
                if (objectTypeState == gCurPlayerData->CurrentTown()) {
                    m_pendingCommand = ADVMGR_COMMAND_TOWN_VIEW;
                    *eventCell = DoAdvCommand();
                } else {
                    SetTownContext(objectTypeState);
                }
            }
            break;
        case ADVENTURE_CONTROL_HERO_PAGE_PREVIOUS:
            if (gCurPlayerData->m_heroLocatorPage > 0) {
                gCurPlayerData->m_heroLocatorPage--;
                UpdateHeroLocators(true, 1);
            }
            break;
        case ADVENTURE_CONTROL_HERO_PAGE_NEXT:
            if (gCurPlayerData->m_heroLocatorPage + LOCATOR_VISIBLE_COUNT
                < gCurPlayerData->m_heroCount) {
                gCurPlayerData->m_heroLocatorPage++;
                UpdateHeroLocators(true, 1);
            }
            break;
        case ADVENTURE_CONTROL_HERO_KNOB:
            DoHeroKnob();
            break;
        case ADVENTURE_CONTROL_HERO_SCROLL:
            gMouseManager->MouseCoords(mouseX, mouseY);
            mouseY -= LOCATOR_SCROLL_MOUSE_BASE_Y;
            if (gCurPlayerData->m_heroCount > LOCATOR_VISIBLE_COUNT) {
                iPage = mouseY
                        / (LOCATOR_SCROLL_MOUSE_SPAN
                           / (gCurPlayerData->m_heroCount - (LOCATOR_VISIBLE_COUNT - 1)));
                if (iPage > gCurPlayerData->m_heroCount - LOCATOR_VISIBLE_COUNT)
                    iPage = gCurPlayerData->m_heroCount - LOCATOR_VISIBLE_COUNT;
            } else {
                iPage = 0;
            }
            gCurPlayerData->m_heroLocatorPage = iPage;
            UpdateHeroLocators(true, 1);
            break;
        case ADVENTURE_CONTROL_TOWN_KNOB:
            DoTownKnob();
            break;
        case ADVENTURE_CONTROL_TOWN_SCROLL:
            gMouseManager->MouseCoords(mouseX, mouseY);
            mouseY -= LOCATOR_SCROLL_MOUSE_BASE_Y;
            if (gCurPlayerData->m_townCount > LOCATOR_VISIBLE_COUNT) {
                iPage = mouseY
                        / (LOCATOR_SCROLL_MOUSE_SPAN
                           / (gCurPlayerData->m_townCount - (LOCATOR_VISIBLE_COUNT - 1)));
                if (iPage > gCurPlayerData->m_townCount - LOCATOR_VISIBLE_COUNT)
                    iPage = gCurPlayerData->m_townCount - LOCATOR_VISIBLE_COUNT;
            } else {
                iPage = 0;
            }
            gCurPlayerData->m_townLocatorPage = iPage;
            UpdateTownLocators(true, 1);
            break;
        case ADVENTURE_CONTROL_TOWN_PAGE_PREVIOUS:
            if (gCurPlayerData->m_townLocatorPage > 0) {
                gCurPlayerData->m_townLocatorPage--;
                UpdateTownLocators(true, 1);
            }
            break;
        case ADVENTURE_CONTROL_TOWN_PAGE_NEXT:
            if (gCurPlayerData->m_townLocatorPage + LOCATOR_VISIBLE_COUNT
                < gCurPlayerData->m_townCount) {
                gCurPlayerData->m_townLocatorPage++;
                UpdateTownLocators(true, 1);
            }
            break;
        case ADVENTURE_CONTROL_MAP_VIEW:
            // At the map's edge the view shows the border beyond it. The
            // original looked such a cell up in the visibility table outside
            // the grid; it now counts as unexplored, so a right click shows
            // the border text and a left click does nothing.
            if (m_mapOriginX + m_hoverCellX < 0 || m_mapOriginY + m_hoverCellY < 0
                || m_mapOriginX + m_hoverCellX >= MAP_CELL_GRID_SIZE
                || m_mapOriginY + m_hoverCellY >= MAP_CELL_GRID_SIZE)
                visible = false;
            else if (!(gGame->m_mapExtra[m_mapOriginX + m_hoverCellX][m_mapOriginY + m_hoverCellY]
                       & gCurPlayerBit))
                visible = false;
            theCell = GetCell(m_mapOriginX + m_hoverCellX, m_mapOriginY + m_hoverCellY);
            // A hero or town cell without the record it names (see
            // game::CellHasRecord) is neither shown nor selected.
            hasRecord = gGame->CellHasRecord(m_mapOriginX + m_hoverCellX, m_mapOriginY + m_hoverCellY);
            if (message->modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON) {
                if (!visible) {
                    QuickInfo(m_hoverCellX, m_hoverCellY);
                } else {
                    if (m_hoverCellX == ADVMGR_VIEW_CENTER && m_hoverCellY == ADVMGR_VIEW_CENTER
                        && gCurPlayerData->CurrentHero() != HERO_ID_NONE && m_heroContextLocked) {
                        objectTypeState = MAP_OBJECT_TRIGGER(MAP_OBJECT_HERO);
                        objectIdIndex = gCurPlayerData->CurrentHero();
                        hasRecord = true;
                    } else {
                        objectTypeState = theCell->m_triggerType & MAP_TRIGGER_TYPE_MASK;
                        objectIdIndex = theCell->m_objectMetadata;
                    }
                    switch (MAP_PASSIVE_OBJECT(objectTypeState)) {
                        case MAP_OBJECT_HERO:
                            if (!hasRecord)
                                break;
                            mouseX = m_hoverCellX * CELL_PIXELS - HERO_QUICK_VIEW_X_OFFSET;
                            if (mouseX < BORDER_EDGE_SIZE)
                                mouseX = BORDER_EDGE_SIZE;
                            if (mouseX + HERO_QUICK_VIEW_WIDTH > BORDER_MIDDLE_END)
                                mouseX = HERO_QUICK_VIEW_RIGHT_X;
                            mouseY = m_hoverCellY * CELL_PIXELS - HERO_QUICK_VIEW_Y_OFFSET;
                            if (mouseY < BORDER_EDGE_SIZE)
                                mouseY = BORDER_EDGE_SIZE;
                            if (mouseY + HERO_QUICK_VIEW_HEIGHT > BORDER_MIDDLE_END)
                                mouseY = HERO_QUICK_VIEW_BOTTOM_Y;
                            HeroQuickView(objectIdIndex, QUICK_VIEW_NO_LOCATOR, mouseX, mouseY);
                            break;
                        case MAP_OBJECT_TOWN:
                            if (!hasRecord)
                                break;
                            mouseX = m_hoverCellX * CELL_PIXELS - TOWN_QUICK_VIEW_X_OFFSET;
                            if (mouseX < BORDER_EDGE_SIZE)
                                mouseX = BORDER_EDGE_SIZE;
                            if (mouseX + TOWN_QUICK_VIEW_WIDTH > BORDER_MIDDLE_END)
                                mouseX = TOWN_QUICK_VIEW_RIGHT_X;
                            mouseY = m_hoverCellY * CELL_PIXELS - TOWN_QUICK_VIEW_Y_OFFSET;
                            if (mouseY < BORDER_EDGE_SIZE)
                                mouseY = BORDER_EDGE_SIZE;
                            if (mouseY + TOWN_QUICK_VIEW_HEIGHT > BORDER_MIDDLE_END)
                                mouseY = TOWN_QUICK_VIEW_BOTTOM_Y;
                            TownQuickView(objectIdIndex, QUICK_VIEW_NO_LOCATOR, mouseX, mouseY);
                            break;
                        default:
                            if (gGame->m_mapExtra[m_mapOriginX + m_hoverCellX]
                                                 [m_mapOriginY + m_hoverCellY]
                                & gCurPlayerBit)
                                QuickInfo(m_hoverCellX, m_hoverCellY);
                            break;
                    }
                }
            } else if (visible) {
                currentHero = NULL;
                mobileResult = false;
                if (gCurPlayerData->m_currentHero != HERO_ID_NONE) {
                    currentHero = gGame->GetHero(gCurPlayerData->m_currentHero);
                    mobileResult = gGame->IsMobile(currentHero->m_id);
                }
                if (currentHero) {
                    if (m_hoverCellX == ADVMGR_VIEW_CENTER && m_hoverCellY == ADVMGR_VIEW_CENTER
                        && gCurPlayerData->CurrentHero() != HERO_ID_NONE && m_heroContextLocked) {
                        m_pendingCommand = ADVMGR_COMMAND_HERO_VIEW;
                        DoAdvCommand();
                    } else if ((!mobileResult
                                || (message->modifiers & MESSAGE_MODIFIER_CONTROL_KEYS)
                                || (gConfig.showRoute
                                    && (currentHero->m_destinationX != m_commandTargetX
                                        || currentHero->m_destinationY != m_commandTargetY)))
                               && gSearchArray->m_cells[m_commandTargetX][m_commandTargetY]
                                      .visited) {
                        currentHero->m_destinationX = m_commandTargetX;
                        currentHero->m_destinationY = m_commandTargetY;
                        ShowRoute(true, 1, true);
                    } else {
                        *eventCell = DoAdvCommand();
                    }
                } else {
                    objectTypeState = theCell->m_triggerType & MAP_TRIGGER_TYPE_MASK;
                    objectIdIndex = theCell->m_objectMetadata;
                    if (MAP_PASSIVE_OBJECT(objectTypeState) == MAP_OBJECT_HERO && hasRecord) {
                        if (objectIdIndex == gCurPlayerData->CurrentHero()) {
                            m_pendingCommand = ADVMGR_COMMAND_HERO_VIEW;
                            DoAdvCommand();
                        } else if (gGame->GetHero(objectIdIndex)->m_owner == gCurPlayer) {
                            SetHeroContext(objectIdIndex, false);
                        }
                    }
                    if (MAP_PASSIVE_OBJECT(objectTypeState) == MAP_OBJECT_TOWN && hasRecord) {
                        if (objectIdIndex == gCurPlayerData->CurrentTown()) {
                            m_pendingCommand = ADVMGR_COMMAND_TOWN_VIEW;
                            *eventCell = DoAdvCommand();
                        } else if (gGame->GetTown(objectIdIndex)->m_owner == gCurPlayer) {
                            SetTownContext(objectIdIndex);
                        }
                    }
                }
            }
            break;
        case ADVENTURE_CONTROL_RADAR:
            if (message->modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON) {
                NormalDialog(
                    localization::Tr("adventure.world_map_help"),
                    NORMAL_DIALOG_TYPE_QUICK_VIEW
                );
                break;
            }
            DemobilizeCurrHero();
            gMouseManager->MouseCoords(mouseX, mouseY);
            mouseX = (mouseX - RADAR_LEFT) / RADAR_CELL_PIXELS;
            mouseY = (mouseY - RADAR_TOP) / RADAR_CELL_PIXELS;
            m_mapOriginX = mouseX - ADVMGR_VIEW_CENTER;
            m_mapOriginY = mouseY - ADVMGR_VIEW_CENTER;
            if (m_mapOriginX < SCROLL_MIN_ORIGIN)
                m_mapOriginX = SCROLL_MIN_ORIGIN;
            if (m_mapOriginY < SCROLL_MIN_ORIGIN)
                m_mapOriginY = SCROLL_MIN_ORIGIN;
            if (m_mapOriginX > SCROLL_MAX_ORIGIN)
                m_mapOriginX = SCROLL_MAX_ORIGIN;
            if (m_mapOriginY > SCROLL_MAX_ORIGIN)
                m_mapOriginY = SCROLL_MAX_ORIGIN;
            UpdateRadar(true, false);
            CompleteDraw(false);
            UpdateScreen(false, false);
            inputMessage.type = MESSAGE_NONE;
            while (inputMessage.type != MESSAGE_LEFT_BUTTON_UP) {
                Process1WindowsMessage();
                inputMessage = gInputManager->GetEvent();
                mouseMsg = inputMessage;
                while (inputMessage.type != MESSAGE_LEFT_BUTTON_UP
                       && inputMessage.type != MESSAGE_NONE) {
                    if (inputMessage.type == MESSAGE_MOUSE_MOVE)
                        mouseMsg = inputMessage;
                    Process1WindowsMessage();
                    inputMessage = gInputManager->GetEvent();
                }
                if (mouseMsg.type == MESSAGE_MOUSE_MOVE) {
                    if (mouseMsg.x < RADAR_LEFT)
                        mouseMsg.x = RADAR_LEFT;
                    if (mouseMsg.x >= RADAR_RIGHT)
                        mouseMsg.x = RADAR_RIGHT - 1;
                    if (mouseMsg.y < RADAR_TOP)
                        mouseMsg.y = RADAR_TOP;
                    if (mouseMsg.y >= RADAR_BOTTOM)
                        mouseMsg.y = RADAR_BOTTOM - 1;
                    gMouseManager->Main(mouseMsg);
                    mouseX = (mouseMsg.x - RADAR_LEFT) / RADAR_CELL_PIXELS;
                    mouseY = (mouseMsg.y - RADAR_TOP) / RADAR_CELL_PIXELS;
                    m_mapOriginX = mouseX - ADVMGR_VIEW_CENTER;
                    m_mapOriginY = mouseY - ADVMGR_VIEW_CENTER;
                    if (m_mapOriginX < SCROLL_MIN_ORIGIN)
                        m_mapOriginX = SCROLL_MIN_ORIGIN;
                    if (m_mapOriginY < SCROLL_MIN_ORIGIN)
                        m_mapOriginY = SCROLL_MIN_ORIGIN;
                    if (m_mapOriginX > SCROLL_MAX_ORIGIN)
                        m_mapOriginX = SCROLL_MAX_ORIGIN;
                    if (m_mapOriginY > SCROLL_MAX_ORIGIN)
                        m_mapOriginY = SCROLL_MAX_ORIGIN;
                    UpdateRadar(true, false);
                    CompleteDraw(false);
                    UpdateScreen(false, false);
                    mouseMsg.type = MESSAGE_NONE;
                }
            }
            break;
        default:
            break;
    }
    if ((message->modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
        && message->id >= BOTTOM_VIEW_DRAW_FIRST_WIDGET
        && message->id <= BOTTOM_VIEW_DRAW_LAST_WIDGET)
        NormalDialog(localization::Tr("adventure.status_help"), NORMAL_DIALOG_TYPE_QUICK_VIEW);
    return MESSAGE_DISPATCH_CONSUME;
}

i32 advManager::ProcessDeSelect(
    struct tag_message* message,
    i32* result,
    class mapCell** eventCell
) {
    switch (message->id) {
        case ADVENTURE_CONTROL_CONTINUE_ROUTE:
            m_pendingCommand = ADVMGR_COMMAND_CONTINUE_ROUTE;
            *eventCell = DoAdvCommand();
            break;
        case ADVENTURE_CONTROL_ADVENTURE_OPTIONS:
            AdvPanel();
            break;
        case ADVENTURE_CONTROL_GAME_OPTIONS:
            *result = ControlPanel();
            break;
        case ADVENTURE_CONTROL_END_TURN:
            if (gCurPlayerData->HasMobileHero()) {
                NormalDialog(
                    localization::Tr("adventure.confirm_end_turn"),
                    NORMAL_DIALOG_TYPE_YES_NO
                );
                if (gWindowManager->m_dialogResult == NORMAL_DIALOG_CANCEL)
                    break;
            }
            gGame->NextPlayer();
            break;
        case ADVENTURE_CONTROL_NEXT_HERO:
            HideRoute(true, false, true);
            SetHeroContext(gCurPlayerData->NextHero(1), false);
            break;
        case ADVENTURE_CONTROL_OVERVIEW:
            gGame->Overview();
            RedrawAdvScreen(true);
            gWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_SHORT, NULL);
            break;
    }
    if (message->id >= BOTTOM_VIEW_DRAW_FIRST_WIDGET
        && message->id <= BOTTOM_VIEW_DRAW_LAST_WIDGET) {
        if (gBottomViewOverride == BOTTOM_VIEW_KINGDOM)
            gBottomViewOverride = BOTTOM_VIEW_NEW_TURN;
        else if (gBottomViewOverride != BOTTOM_VIEW_NONE)
            gBottomViewOverride = BOTTOM_VIEW_NONE;
        else if (gCurBottomView == BOTTOM_VIEW_KINGDOM)
            gBottomViewOverride = BOTTOM_VIEW_NEW_TURN;
        else
            gBottomViewOverride = BOTTOM_VIEW_KINGDOM;
        gBottomViewOverrideEndTime = KBTickCount() + 3000;
        UpdBottomView(true, true, true);
    }
    return MESSAGE_DISPATCH_CONSUME;
}

b32 advManager::ProcessSearch(i32 x, i32 y) {
    class sample* sample = NULL;
    hero* hero;
    i32 pl;
    mapCell* cellPtr;
    tag_message evt;
    i32 gaveArtifact;

    // The search key works without a selected hero too; the original then
    // searched with hero -1, the bytes before the hero table.
    if (gCurPlayerData->m_currentHero == HERO_ID_NONE)
        return 1;
    hero = gGame->GetHero(gCurPlayerData->m_currentHero);
    if (hero->m_remainingMobility != hero->m_mobility
        && !gCheatUnlimitedMovement[gCurPlayer][hero->m_id]) {
        NormalDialog(localization::Tr("adventure.search.requires_full_day"), NORMAL_DIALOG_TYPE_OK);
        return true;
    }
    MobilizeCurrHero(false);
    CompleteDraw(false);
    UpdateScreen(false, false);
    if (x == ADVMGR_SEARCH_VIEW_CENTER) {
        x = m_mapOriginX + ADVMGR_VIEW_CENTER;
        y = m_mapOriginY + ADVMGR_VIEW_CENTER;
    }
    cellPtr = GetCell(x, y);
    if (cellPtr->m_objectIndex != MAP_CELL_NO_FRAME
        || cellPtr->m_overlayIndex != MAP_CELL_NO_FRAME) {
        NormalDialog(localization::Tr("adventure.search.clear_ground"), NORMAL_DIALOG_TYPE_OK);
        return true;
    }
    if (cellPtr->m_tileIndex < MAP_CELL_TILES_PER_TERRAIN) {
        NormalDialog(localization::Tr("adventure.search.on_land"), NORMAL_DIALOG_TYPE_OK);
        return true;
    }
    if (gHumanPlayer[gCurPlayer])
        sample = LoadPlaySample("DIGSOUND.82M");
    if (cellPtr->m_objectIndex == MAP_CELL_NO_FRAME) {
        cellPtr->m_objectTileset = TILESET_OBJ32_07;
        cellPtr->m_objectIndex = DIG_HOLE_FRAME;
        cellPtr->m_flags |= MAP_CELL_OBJECT_SHADOW_ONLY;
    }
    CompleteDraw(false);
    UpdateScreen(false, false);
    GrabScreen();

    if (gGame->m_ultimateArtifactX == x && gGame->m_ultimateArtifactY == y
        && gGame->m_ultimateArtifactId != ARTIFACT_NONE) {
        gaveArtifact = GiveArtifact(hero, gGame->m_ultimateArtifactId);
        if (gaveArtifact == GIVE_ARTIFACT_NO_SLOT) {
            NormalDialog(
                localization::Tr("adventure.search.no_artifact_room"),
                NORMAL_DIALOG_TYPE_OK,
                0x61,
                0x28
            );
        } else {
            if (gHumanPlayer[gCurPlayer]) {
                EventSound(MAP_OBJECT_ULTIMATE_ARTIFACT, 0);
                sprintf(
                    gText,
                    localization::Tr("adventure.search.found_format"),
                    localization::Tr("adventure.search.found_prefix"),
                    gArtifactNames[gGame->m_ultimateArtifactId]
                );
                if (gGame->m_campaignType > 0
                    && gGame->m_campaignScenario == CAMPAIGN_SCENARIO_EYE_OF_GOROS) {
                    sprintf(gText, localization::Tr("adventure.search.eye_of_goros_found"));
                    NormalDialog(gText, NORMAL_DIALOG_TYPE_OK, 0xb1, 0x1c);
                } else {
                    NormalDialog(gText, NORMAL_DIALOG_TYPE_OK, 0xb1, 0x1c);
                    hero->ViewArtifact(gGame->m_ultimateArtifactId, 0);
                }
                PlayMusic(TERRAIN_MUSIC_TRACK(m_currentTerrain));
            } else if (gGame->m_campaignType > 0
                       && gGame->m_campaignScenario == CAMPAIGN_SCENARIO_EYE_OF_GOROS) {
                sprintf(gText, localization::Tr("adventure.search.eye_of_goros_enemy"));
                NormalDialog(gText, NORMAL_DIALOG_TYPE_OK, 0xb1, 0x1c);
            }
            gGame->m_ultimateArtifactId = ARTIFACT_NONE;
        }
    } else if (gHumanPlayer[gCurPlayer]) {
        NormalDialog(
            localization::Tr("adventure.search.nothing_here"),
            NORMAL_DIALOG_TYPE_OK,
            0x61,
            0x28
        );
    }
    if (gHumanPlayer[gCurPlayer])
        WaitSample(sample);
    for (pl = 0; pl < gGame->m_playerCount; pl++)
        ComputeUALoc(pl);
    if (!gCheatUnlimitedMovement[gCurPlayer][hero->m_id])
        hero->m_remainingMobility = 0;
    UpdBottomView(true, true, true);
    CheckDimHero();
    Reseed(0, 0);
    CheckEndGame(false);
    return true;
}

i32 advManager::ProcessHover(struct tag_message* message) {
    i8 trigType;
    i32 baseFrame;
    i32 nDays;
    hero* curHero;
    mapCell* hoverCell;
    town* pTown;
    i16 yPos;
    i16 xPos;
    i16 heroPosY;
    i16 heroPosX;
    b32 hasRecord;

    switch (message->id) {
        case ADVENTURE_CONTROL_MAP_VIEW:
            gMouseManager->MouseCoords(xPos, yPos);
            if (xPos > ADVENTURE_VIEWPORT_EXTENT) {
                gMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
                return MESSAGE_DISPATCH_CONSUME;
            }
            xPos = xPos / CELL_PIXELS;
            yPos = yPos / CELL_PIXELS;
            if (xPos < 0)
                xPos = 0;
            if (yPos < 0)
                yPos = 0;
            if (xPos > ADVMGR_VIEW_CELL_COUNT - 1)
                xPos = ADVMGR_VIEW_CELL_COUNT - 1;
            if (yPos > ADVMGR_VIEW_CELL_COUNT - 1)
                yPos = ADVMGR_VIEW_CELL_COUNT - 1;
            if (m_hoverCellX != xPos || m_hoverCellY != yPos) {
                m_pendingCommand = ADVMGR_COMMAND_NONE;
                m_hoverCellX = xPos;
                m_hoverCellY = yPos;
                m_commandTargetX = m_mapOriginX + xPos;
                m_commandTargetY = m_mapOriginY + yPos;
                if (m_commandTargetX < 0 || m_commandTargetY < 0
                    || m_commandTargetX > MAP_CELL_GRID_SIZE - 1
                    || m_commandTargetY > MAP_CELL_GRID_SIZE - 1
                    || !(gGame->m_mapExtra[m_commandTargetX][m_commandTargetY] & gCurPlayerBit)) {
                    gMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
                    return MESSAGE_DISPATCH_CONSUME;
                }
                hoverCell = GetCell(m_commandTargetX, m_commandTargetY);
                // A hero or town cell without the record it names (see
                // game::CellHasRecord) is hovered as ground.
                hasRecord = gGame->CellHasRecord(m_commandTargetX, m_commandTargetY);
                if (gCurPlayerData->m_currentHero == HERO_ID_NONE) {
                    if (MAP_TRIGGER_OBJECT(hoverCell->m_triggerType) == MAP_OBJECT_TOWN && hasRecord
                        && gGame->GetTown(hoverCell->m_objectMetadata)->m_owner == gCurPlayer) {
                        gMouseManager->SetPointer(ADVENTURE_POINTER_TOWN);
                        m_pendingCommand = ADVMGR_COMMAND_TOWN_VIEW;
                        return MESSAGE_DISPATCH_CONSUME;
                    } else if (MAP_TRIGGER_OBJECT(hoverCell->m_triggerType) == MAP_OBJECT_HERO
                               && hasRecord
                               && gGame->GetHero(hoverCell->m_objectMetadata)->m_owner
                                      == gCurPlayer) {
                        gMouseManager->SetPointer(ADVENTURE_POINTER_HERO);
                        m_pendingCommand = ADVMGR_COMMAND_HERO_VIEW;
                        return MESSAGE_DISPATCH_CONSUME;
                    } else {
                        gMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
                        return MESSAGE_DISPATCH_CONSUME;
                    }
                } else {
                    curHero = gGame->GetHero(gCurPlayerData->m_currentHero);
                    heroPosX = curHero->m_x - m_mapOriginX;
                    heroPosY = curHero->m_y - m_mapOriginY;
                    if (xPos == heroPosX && yPos == heroPosY) {
                        gMouseManager->SetPointer(ADVENTURE_POINTER_HERO);
                        m_pendingCommand = ADVMGR_COMMAND_HERO_VIEW;
                        return MESSAGE_DISPATCH_CONSUME;
                    }
                    if (hoverCell->m_secondaryTrigger & MAP_CELL_SECONDARY_BLOCKED) {
                        if (MAP_TRIGGER_OBJECT(hoverCell->m_triggerType) == MAP_OBJECT_TOWN
                            && hasRecord) {
                            pTown = gGame->GetTown(hoverCell->m_objectMetadata);
                            if (pTown->m_owner == gCurPlayer && m_commandTargetY >= 1
                                && m_commandTargetY < MAP_CELL_GRID_SIZE - 1
                                && (MAP_TRIGGER_OBJECT(
                                        GetCell(m_commandTargetX, m_commandTargetY - 1)
                                            ->m_triggerType
                                    ) == MAP_OBJECT_TOWN
                                    || MAP_TRIGGER_OBJECT(
                                           GetCell(m_commandTargetX, m_commandTargetY - 1)
                                               ->m_secondaryTrigger
                                       ) == MAP_OBJECT_TOWN)
                                && (MAP_TRIGGER_OBJECT(
                                        GetCell(m_commandTargetX, m_commandTargetY + 1)
                                            ->m_triggerType
                                    ) == MAP_OBJECT_TOWN
                                    || MAP_TRIGGER_OBJECT(
                                           GetCell(m_commandTargetX, m_commandTargetY + 1)
                                               ->m_secondaryTrigger
                                       ) == MAP_OBJECT_TOWN)) {
                                gMouseManager->SetPointer(ADVENTURE_POINTER_TOWN);
                                m_pendingCommand = ADVMGR_COMMAND_SELECT_TOWN;
                                return MESSAGE_DISPATCH_CONSUME;
                            }
                        }
                        gSearchArray->m_pathLength = 0;
                        gMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
                        return MESSAGE_DISPATCH_CONSUME;
                    }
                    if (!((m_cursorType == ADVMGR_HERO_ICON_BOAT
                           || hoverCell->m_tileIndex >= MAP_CELL_TILES_PER_TERRAIN
                           || hoverCell->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_HERO)
                           || hoverCell->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_SHIP)
                           || hoverCell->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_SHIPWRECK))
                          && (m_cursorType != ADVMGR_HERO_ICON_BOAT
                              || hoverCell->m_tileIndex < MAP_CELL_TILES_PER_TERRAIN
                              || hoverCell->m_triggerType
                                     == MAP_OBJECT_TRIGGER(MAP_OBJECT_COAST)))) {
                        gSearchArray->m_pathLength = 0;
                        gMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
                        return MESSAGE_DISPATCH_CONSUME;
                    }
                    SeedTo(m_commandTargetX, m_commandTargetY);
                    if (gSearchArray->m_cells[m_commandTargetX][m_commandTargetY].visited) {
                        if (gSearchArray->m_cells[m_commandTargetX][m_commandTargetY].distance
                            <= curHero->m_remainingMobility) {
                            nDays = 0;
                        } else {
                            nDays =
                                (gSearchArray->m_cells[m_commandTargetX][m_commandTargetY].distance
                                 - curHero->m_remainingMobility)
                                    / curHero->m_mobility
                                + 1;
                            if (nDays > ADVENTURE_POINTER_DAY_LAST)
                                nDays = ADVENTURE_POINTER_DAY_LAST;
                        }
                        baseFrame = nDays * ADVENTURE_POINTER_DAY_STRIDE;
                        switch (MAP_TRIGGER_OBJECT(hoverCell->m_triggerType)) {
                            case MAP_OBJECT_SHIP:
                                if (m_cursorType != ADVMGR_HERO_ICON_BOAT) {
                                    gMouseManager->SetPointer(baseFrame + ADVENTURE_POINTER_SAIL);
                                    m_pendingCommand = ADVMGR_COMMAND_MOVE_TO;
                                } else {
                                    gMouseManager->SetPointer(baseFrame);
                                }
                                break;
                            case MAP_OBJECT_COAST:
                                if (m_cursorType == ADVMGR_HERO_ICON_BOAT)
                                    gMouseManager->SetPointer(
                                        baseFrame + ADVENTURE_POINTER_DISEMBARK
                                    );
                                else if (gMapExtra[m_commandTargetX][m_commandTargetY]
                                         & MAP_EXTRA_MONSTER_ADJACENT)
                                    gMouseManager->SetPointer(baseFrame + ADVENTURE_POINTER_ATTACK);
                                else
                                    gMouseManager->SetPointer(baseFrame + ADVENTURE_POINTER_MOVE);
                                m_pendingCommand = ADVMGR_COMMAND_MOVE_TO;
                                break;
                            case MAP_OBJECT_MONSTER:
                                gMouseManager->SetPointer(baseFrame + ADVENTURE_POINTER_ATTACK);
                                m_pendingCommand = ADVMGR_COMMAND_MOVE_TO;
                                break;
                            case MAP_OBJECT_HERO:
                                if (!hasRecord)
                                    goto defaultHover;
                                if (gGame->GetHero(hoverCell->m_objectMetadata)->m_owner
                                    != gCurPlayer) {
                                    gMouseManager->SetPointer(baseFrame + ADVENTURE_POINTER_ATTACK);
                                    m_pendingCommand = ADVMGR_COMMAND_MOVE_TO;
                                } else {
                                    gMouseManager->SetPointer(
                                        baseFrame + ADVENTURE_POINTER_SELECT_HERO
                                    );
                                    m_pendingCommand = ADVMGR_COMMAND_MOVE_TO;
                                }
                                break;
                            case MAP_OBJECT_TOWN:
                                if (!hasRecord)
                                    goto defaultHover;
                                pTown = gGame->GetTown(hoverCell->m_objectMetadata);
                                if ((hoverCell->m_triggerType & MAP_TRIGGER_EVENT)
                                    && pTown->m_owner != gCurPlayer && pTown->HasGarrison()) {
                                    gMouseManager->SetPointer(baseFrame + ADVENTURE_POINTER_ATTACK);
                                    m_pendingCommand = ADVMGR_COMMAND_MOVE_TO;
                                    break;
                                }
                                goto defaultHover;
                            default:
                            defaultHover:
                                trigType = MAP_TRIGGER_OBJECT(hoverCell->m_triggerType);
                                if ((gMapExtra[m_commandTargetX][m_commandTargetY]
                                     & MAP_EXTRA_MONSTER_ADJACENT)
                                    && m_cursorType != ADVMGR_HERO_ICON_BOAT
                                    && trigType != MAP_OBJECT_SKELETON
                                    && trigType != MAP_OBJECT_TREASURE_CHEST
                                    && trigType != MAP_OBJECT_CAMPFIRE
                                    && trigType != MAP_OBJECT_ANCIENT_LAMP
                                    && trigType != MAP_OBJECT_RESOURCE
                                    && trigType != MAP_OBJECT_ARTIFACT) {
                                    gMouseManager->SetPointer(baseFrame + ADVENTURE_POINTER_ATTACK);
                                } else if (hoverCell->m_triggerType & MAP_TRIGGER_EVENT) {
                                    if (m_cursorType != ADVMGR_HERO_ICON_BOAT) {
                                        switch (MAP_TRIGGER_OBJECT(hoverCell->m_triggerType)) {
                                            case MAP_OBJECT_ALCHEMIST_LAB:
                                            case MAP_OBJECT_SIGNPOST:
                                            case MAP_OBJECT_SKELETON:
                                            case MAP_OBJECT_DAEMON_CAVE:
                                            case MAP_OBJECT_TREASURE_CHEST:
                                            case MAP_OBJECT_FAERIE_RING:
                                            case MAP_OBJECT_CAMPFIRE:
                                            case MAP_OBJECT_FOUNTAIN:
                                            case MAP_OBJECT_GAZEBO:
                                            case MAP_OBJECT_ANCIENT_LAMP:
                                            case MAP_OBJECT_GRAVEYARD:
                                            case MAP_OBJECT_STRAW_HUT:
                                            case MAP_OBJECT_HOUSE:
                                            case MAP_OBJECT_CABIN:
                                            case MAP_OBJECT_DWARF_LOG_CABIN:
                                            case MAP_OBJECT_PEASANT_LOG_CABIN:
                                            case MAP_OBJECT_INN_1:
                                            case MAP_OBJECT_INN_2:
                                            case MAP_OBJECT_INN_3:
                                            case MAP_OBJECT_INN_4:
                                            case MAP_OBJECT_DRAGON_CITY:
                                            case MAP_OBJECT_LIGHTHOUSE:
                                            case MAP_OBJECT_WATERWHEEL:
                                            case MAP_OBJECT_MINE:
                                            case MAP_OBJECT_OBELISK:
                                            case MAP_OBJECT_OASIS:
                                            case MAP_OBJECT_RESOURCE:
                                            case MAP_OBJECT_SAWMILL:
                                            case MAP_OBJECT_RANKING_SHRINE:
                                            case MAP_OBJECT_SPELL_SHRINE:
                                            case MAP_OBJECT_SHIPWRECK:
                                            case MAP_OBJECT_STATUE:
                                            case MAP_OBJECT_DESERT_TENT:
                                            case MAP_OBJECT_TOWN:
                                            case MAP_OBJECT_STONE_LITHS:
                                            case MAP_OBJECT_WAGON_CAMP:
                                            case MAP_OBJECT_WELL:
                                            case MAP_OBJECT_WHIRLPOOL:
                                            case MAP_OBJECT_WINDMILL:
                                            case MAP_OBJECT_OAK_TREE:
                                            case MAP_OBJECT_MEGALITH:
                                            case MAP_OBJECT_ARTIFACT:
                                                gMouseManager->SetPointer(
                                                    baseFrame + ADVENTURE_POINTER_ACTION
                                                );
                                                break;
                                            default:
                                                if (gMapExtra[m_commandTargetX][m_commandTargetY]
                                                    & MAP_EXTRA_MONSTER_ADJACENT)
                                                    gMouseManager->SetPointer(
                                                        baseFrame + ADVENTURE_POINTER_ATTACK
                                                    );
                                                else
                                                    gMouseManager->SetPointer(
                                                        baseFrame + ADVENTURE_POINTER_MOVE
                                                    );
                                                break;
                                        }
                                    } else {
                                        switch (MAP_TRIGGER_OBJECT(hoverCell->m_triggerType)) {
                                            case MAP_OBJECT_BUOY:
                                            case MAP_OBJECT_WHIRLPOOL:
                                                gMouseManager->SetPointer(
                                                    nDays + ADVENTURE_POINTER_WATER_ACTION
                                                );
                                                break;
                                            default:
                                                gMouseManager->SetPointer(
                                                    baseFrame + ADVENTURE_POINTER_SAIL
                                                );
                                                break;
                                        }
                                    }
                                } else if (m_cursorType == ADVMGR_HERO_ICON_BOAT) {
                                    gMouseManager->SetPointer(baseFrame + ADVENTURE_POINTER_SAIL);
                                } else {
                                    gMouseManager->SetPointer(baseFrame + ADVENTURE_POINTER_MOVE);
                                }
                                m_pendingCommand = ADVMGR_COMMAND_MOVE_TO;
                                break;
                        }
                        return MESSAGE_DISPATCH_CONSUME;
                    } else {
                        gMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
                        return MESSAGE_DISPATCH_CONSUME;
                    }
                }
            }
            break;
        default:
            if (!(gMouseManager->m_cursorFrame >= HOVER_SCROLL_FRAME_FIRST
                  && gMouseManager->m_cursorFrame < HOVER_SCROLL_FRAME_END && MouseInScrollZone()))
                gMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
            return MESSAGE_DISPATCH_CONSUME;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

void advManager::UpdateScreen(b8 cursorUpdate, b8 forceUpdate) {
    if (!forceUpdate && !gShowIt) {
        if (gTimers[ADVENTURE_FRAME_TIMER_SLOT] < KBTickCount())
            gTimers[ADVENTURE_FRAME_TIMER_SLOT] = KBTickCount() + TIMER_DELAY;
        return;
    }
    gMouseManager
        ->SaveAndDraw(gWindowManager->m_screen, m_scrollOffsetX, m_scrollOffsetY, cursorUpdate);
    PollSound();
    gScrollX = m_scrollOffsetX;
    gScrollY = m_scrollOffsetY;
    if (gLimitUpdMinX == UPDATE_NONE)
        BlitBitmapToScreen(
            gWindowManager->m_screen,
            BORDER_EDGE_SIZE,
            BORDER_EDGE_SIZE,
            ADVENTURE_VIEWPORT_INNER_SIZE,
            ADVENTURE_VIEWPORT_INNER_SIZE,
            BORDER_EDGE_SIZE,
            BORDER_EDGE_SIZE
        );
    else
        BlitBitmapToScreen(
            gWindowManager->m_screen,
            gLimitUpdMinX,
            gLimitUpdMinY,
            gLimitUpdMaxX - gLimitUpdMinX,
            gLimitUpdMaxY - gLimitUpdMinY,
            gLimitUpdMinX,
            gLimitUpdMinY
        );
    gScrollY = 0;
    gScrollX = gScrollY;
    PollSound();
    if (gTimers[ADVENTURE_FRAME_TIMER_SLOT] < KBTickCount()) {
        ++m_animationFrame;
        if (m_animationFrame >= UPDATE_FRAME_CYCLE)
            m_animationFrame = 0;
        gTimers[ADVENTURE_FRAME_TIMER_SLOT] = KBTickCount() + TIMER_DELAY;
        if (m_animationFrame == UPDATE_FRAME_STEP_1 || m_animationFrame == UPDATE_FRAME_STEP_3
            || m_animationFrame == UPDATE_FRAME_STEP_5) {
            ++m_animationPhases[ANIMATION_PHASE_COLUMN_1];
            m_animationPhases[ANIMATION_PHASE_COLUMN_1] %= UPDATE_ANIMATION_PHASES;
            ++m_animationPhases[ANIMATION_PHASE_COLUMN_3];
            m_animationPhases[ANIMATION_PHASE_COLUMN_3] %= UPDATE_ANIMATION_PHASES;
        } else {
            ++m_animationPhases[ANIMATION_PHASE_COLUMN_0];
            m_animationPhases[ANIMATION_PHASE_COLUMN_0] %= UPDATE_ANIMATION_PHASES;
            ++m_animationPhases[ANIMATION_PHASE_COLUMN_2];
            m_animationPhases[ANIMATION_PHASE_COLUMN_2] %= UPDATE_ANIMATION_PHASES;
        }
    }
    gLimitUpdMinX = UPDATE_NONE;
    gMouseManager->RestoreUnderlying();
    Process1WindowsMessage();
}

void advManager::CompleteDraw(i16 originX, i16 originY, b32 forceDraw) {
    i32 drawX;
    i32 drawY;

    PollSound();
    if (!forceDraw && !gShowIt)
        return;

    gLimitUpdMinX = UPDATE_NONE;
    m_previousOriginX = m_mapOriginX;
    m_previousOriginY = m_mapOriginY;
    if (gAllBlack)
        m_mapOriginX = m_mapOriginY = 0;
    m_comboHeroDrawn = false;
    m_forceCompleteDraw = false;

    for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++)
        DrawCell(
            originX + drawX,
            originY,
            drawX,
            0,
            ADVMGR_DRAW_GROUND | ADVMGR_DRAW_OBJECT,
            false,
            forceDraw
        );
    for (drawY = 1; drawY < ADVMGR_VIEW_CELL_COUNT; drawY++)
        for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++)
            DrawCell(
                originX + drawX,
                originY + drawY,
                drawX,
                drawY,
                ADVMGR_DRAW_GROUND,
                false,
                forceDraw
            );

    for (drawY = 1; drawY < ADVMGR_VIEW_CELL_COUNT; drawY++) {
        PollSound();
        if (m_cursorDirection > MAP_DIRECTION_UNMIRRORED_LAST) {
            for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++)
                DrawCell(
                    originX + drawX,
                    originY + drawY - 1,
                    drawX,
                    drawY - 1,
                    ADVMGR_DRAW_OVERLAY | ADVMGR_DRAW_HERO,
                    false,
                    forceDraw
                );
        } else {
            for (drawX = ADVMGR_VIEW_CELL_COUNT - 1; drawX >= 0; drawX--)
                DrawCell(
                    originX + drawX,
                    originY + drawY - 1,
                    drawX,
                    drawY - 1,
                    ADVMGR_DRAW_OVERLAY | ADVMGR_DRAW_HERO,
                    false,
                    forceDraw
                );
        }
        for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++)
            DrawCell(
                originX + drawX,
                originY + drawY,
                drawX,
                drawY,
                ADVMGR_DRAW_OBJECT,
                false,
                forceDraw
            );
    }

    for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++)
        DrawCell(
            originX + drawX,
            originY + ADVMGR_VIEW_CELL_COUNT - 1,
            drawX,
            ADVMGR_VIEW_CELL_COUNT - 1,
            ADVMGR_DRAW_OVERLAY | ADVMGR_DRAW_HERO,
            false,
            forceDraw
        );
    for (drawY = 0; drawY < ADVMGR_VIEW_CELL_COUNT; drawY++)
        for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++)
            DrawCell(
                originX + drawX,
                originY + drawY,
                drawX,
                drawY,
                ADVMGR_DRAW_CLOUD,
                false,
                forceDraw
            );

    PollSound();
    UpdBottomView(false, true, true);
    DrawAdventureBorder();
    if (gAllBlack) {
        m_mapOriginX = m_previousOriginX;
        m_mapOriginY = m_previousOriginY;
    }
}

void advManager::CompleteDraw(b32 forceDraw) {
    CompleteDraw(m_mapOriginX, m_mapOriginY, forceDraw);
}

i32 advManager::GetCloudLookup(i32 x, i32 y) {
    i32 cloudMask = 0;

    if (x < 1)
        cloudMask |= CLOUD_WEST_EDGE;
    else if (x >= MAP_CELL_GRID_SIZE - 1)
        cloudMask |= CLOUD_EAST_EDGE;
    if (y < 1)
        cloudMask |= CLOUD_NORTH_EDGE;
    else if (y >= MAP_CELL_GRID_SIZE - 1)
        cloudMask |= CLOUD_SOUTH_EDGE;
    if (cloudMask == 0) {
        if ((gGame->m_mapExtra[x][y - 1] & gCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_NORTH;
        if ((gGame->m_mapExtra[x + 1][y] & gCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_EAST;
        if ((gGame->m_mapExtra[x][y + 1] & gCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_SOUTH;
        if ((gGame->m_mapExtra[x - 1][y] & gCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_WEST;
        if ((gGame->m_mapExtra[x + 1][y - 1] & gCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_NORTH_EAST;
        if ((gGame->m_mapExtra[x + 1][y + 1] & gCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_SOUTH_EAST;
        if ((gGame->m_mapExtra[x - 1][y + 1] & gCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_SOUTH_WEST;
        if ((gGame->m_mapExtra[x - 1][y - 1] & gCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_NORTH_WEST;
    } else {
        if ((cloudMask & CLOUD_NORTH) == 0
            && (gGame->m_mapExtra[x][y - 1] & gCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_NORTH;
        if ((cloudMask & CLOUD_EAST) == 0
            && (gGame->m_mapExtra[x + 1][y] & gCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_EAST;
        if ((cloudMask & CLOUD_SOUTH) == 0
            && (gGame->m_mapExtra[x][y + 1] & gCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_SOUTH;
        if ((cloudMask & CLOUD_WEST) == 0
            && (gGame->m_mapExtra[x - 1][y] & gCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_WEST;
        if ((cloudMask & CLOUD_NORTH_EAST) == 0
            && (gGame->m_mapExtra[x + 1][y - 1] & gCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_NORTH_EAST;
        if ((cloudMask & CLOUD_SOUTH_EAST) == 0
            && (gGame->m_mapExtra[x + 1][y + 1] & gCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_SOUTH_EAST;
        if ((cloudMask & CLOUD_SOUTH_WEST) == 0
            && (gGame->m_mapExtra[x - 1][y + 1] & gCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_SOUTH_WEST;
        if ((cloudMask & CLOUD_NORTH_WEST) == 0
            && (gGame->m_mapExtra[x - 1][y - 1] & gCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_NORTH_WEST;
    }
    return gCloudType[cloudMask];
}

void advManager::DrawCell(
    i16 mapX,
    i16 mapY,
    i16 screenX,
    i16 screenY,
    i8 drawMask,
    b8 drawingPuzzle,
    b8 forceDraw
) {
    i32 heroYOffset;
    i8 playerColor;
    b8 showHero;
    hero* occupyingHero;
    i8 heroIcon;
    i8 heroFrame;
    i32 savedSuppressed;
    mapCell* drawnCell;
    i16 cellPixelY;
    i16 cellPixelX;

    if (!forceDraw && !gShowIt)
        return;
    cellPixelX = screenX << CELL_PIXEL_SHIFT;
    cellPixelY = screenY << CELL_PIXEL_SHIFT;
    drawnCell = GetCell(mapX, mapY);
    if (!gAllBlack
        && (mapX < 0 || mapY < 0 || mapX >= MAP_CELL_GRID_SIZE || mapY >= MAP_CELL_GRID_SIZE)) {
        s_drawStoneTile = STONE_TILE_NONE;
        if (mapX == STONE_BORDER_LOW) {
            if (mapY == STONE_BORDER_LOW)
                s_drawStoneTile = STONE_TILE_TOP_LEFT;
            else if (mapY == MAP_CELL_GRID_SIZE)
                s_drawStoneTile = STONE_TILE_BOTTOM_LEFT;
            else if (mapY >= 0 && mapY < MAP_CELL_GRID_SIZE)
                s_drawStoneTile = ((mapY + STONE_PATTERN_COORDINATE_SHIFT) & CLOUD_VARIANT_MASK)
                                  + STONE_TILE_LEFT_BASE;
        } else if (mapX == MAP_CELL_GRID_SIZE) {
            if (mapY == STONE_BORDER_LOW)
                s_drawStoneTile = STONE_TILE_TOP_RIGHT;
            else if (mapY == MAP_CELL_GRID_SIZE)
                s_drawStoneTile = STONE_TILE_BOTTOM_RIGHT;
            else if (mapY >= 0 && mapY < MAP_CELL_GRID_SIZE)
                s_drawStoneTile = ((mapY + STONE_PATTERN_COORDINATE_SHIFT) & CLOUD_VARIANT_MASK)
                                  + STONE_TILE_RIGHT_BASE;
        } else if (mapY == STONE_BORDER_LOW) {
            if (mapX >= 0 && mapX < MAP_CELL_GRID_SIZE)
                s_drawStoneTile = ((mapX + STONE_PATTERN_COORDINATE_SHIFT) & CLOUD_VARIANT_MASK)
                                  + STONE_TILE_TOP_BASE;
        } else if (mapY == MAP_CELL_GRID_SIZE && mapX >= 0 && mapX < MAP_CELL_GRID_SIZE) {
            s_drawStoneTile = ((mapX + STONE_PATTERN_COORDINATE_SHIFT) & CLOUD_VARIANT_MASK)
                              + STONE_TILE_BOTTOM_BASE;
        }
        if (s_drawStoneTile == STONE_TILE_NONE)
            s_drawStoneTile =
                (mapX + STONE_PATTERN_COORDINATE_SHIFT) % CLOUD_VARIANTS
                + ((mapY + STONE_PATTERN_COORDINATE_SHIFT) % CLOUD_VARIANTS) * CLOUD_VARIANTS;
        TileToBitmap(
            m_stoneTiles,
            s_drawStoneTile,
            gWindowManager->m_screen,
            cellPixelX,
            cellPixelY
        );
        return;
    } else {
        if (!((!gAllBlack && (gGame->m_mapExtra[mapX][mapY] & gCurWatchPlayerBit))
              || drawingPuzzle)) {
            s_drawCovered = true;
            if (gAllBlack)
                s_drawCloudFrame = 0;
            else
                s_drawCloudFrame = GetCloudLookup(mapX, mapY);
            if (s_drawCloudFrame == 0) {
                if (drawMask & ADVMGR_DRAW_CLOUD)
                    TileToBitmap(
                        m_cloudTiles,
                        (mapX + mapY) & CLOUD_VARIANT_MASK,
                        gWindowManager->m_screen,
                        cellPixelX,
                        cellPixelY
                    );
                return;
            }
            if (s_drawCloudFrame >= CLOUD_FLIPPED_FRAME_BASE) {
                s_drawFlipCloud = true;
                s_drawCloudFrame -= CLOUD_FLIPPED_FRAME_BASE;
            } else {
                s_drawFlipCloud = false;
            }
            if ((s_drawCloudFrame == CLOUD_X_ALTERNATE_FRAME_1
                 || s_drawCloudFrame == CLOUD_X_ALTERNATE_FRAME_2)
                && (mapX & 1))
                s_drawCloudFrame++;
            if (s_drawCloudFrame == CLOUD_Y_ALTERNATE_FRAME && (mapY & 1))
                s_drawCloudFrame++;
        } else {
            s_drawCovered = false;
        }
    }
    if (drawMask & ADVMGR_DRAW_CLOUD) {
        if (s_drawCovered) {
            if (s_drawFlipCloud)
                FlipIconToBitmap(
                    m_cloudOverlayIcon,
                    gWindowManager->m_screen,
                    cellPixelX + CELL_LAST_PIXEL,
                    cellPixelY,
                    s_drawCloudFrame - 1,
                    ICON_DRAW_OFFSET_FULL
                );
            else
                IconToBitmap(
                    m_cloudOverlayIcon,
                    gWindowManager->m_screen,
                    cellPixelX,
                    cellPixelY,
                    s_drawCloudFrame - 1,
                    ICON_DRAW_OFFSET_FULL
                );
        } else if (m_routeShown && ADVMGR_ROUTE_AT(mapX, mapY)) {
            if (ADVMGR_ROUTE_AT(mapX, mapY) & ROUTE_CELL_FLIPPED)
                FlipIconToBitmap(
                    m_objectIcons[TILESET_ROUTE],
                    gWindowManager->m_screen,
                    cellPixelX + CELL_LAST_PIXEL,
                    cellPixelY + ROUTE_DRAW_Y_OFFSET,
                    (ADVMGR_ROUTE_AT(mapX, mapY) & ROUTE_CELL_FRAME_MASK) - 1,
                    ICON_DRAW_OFFSET_FULL
                );
            else
                IconToBitmap(
                    m_objectIcons[TILESET_ROUTE],
                    gWindowManager->m_screen,
                    cellPixelX,
                    cellPixelY + ROUTE_DRAW_Y_OFFSET,
                    (ADVMGR_ROUTE_AT(mapX, mapY) & ROUTE_CELL_FRAME_MASK) - 1,
                    ICON_DRAW_OFFSET_FULL
                );
        }
        return;
    }
    if (drawMask & ADVMGR_DRAW_GROUND) {
        s_drawGroundTile = drawnCell->m_flags;
        s_drawGroundTile <<= MAP_CELL_GROUND_FLIP_SHIFT;
        s_drawGroundTile |= drawnCell->m_tileIndex & 0xff;
        TileToBitmap(
            m_groundTiles,
            s_drawGroundTile,
            gWindowManager->m_screen,
            cellPixelX,
            cellPixelY
        );
        if (drawnCell->m_flags & MAP_CELL_OBJECT_SHADOW_ONLY) {
            s_drawTileset =
                (drawnCell->m_objectTileset & MAP_CELL_TILESET_MASK);
            if (!drawingPuzzle || s_drawTileset != TILESET_OBJ32_07
                || drawnCell->m_objectIndex != DIG_HOLE_FRAME)
                IconToBitmap(
                    m_objectIcons[s_drawTileset],
                    gWindowManager->m_screen,
                    cellPixelX,
                    cellPixelY,
                    drawnCell->m_objectIndex,
                    ICON_DRAW_OFFSET_FULL
                );
        }
    }
    if (drawMask & ADVMGR_DRAW_OBJECT) {
        if (!(drawnCell->m_flags & MAP_CELL_OBJECT_SHADOW_ONLY)
            && drawnCell->m_objectIndex != MAP_CELL_NO_FRAME) {
            s_drawTileset =
                (drawnCell->m_objectTileset & MAP_CELL_TILESET_MASK);
            if (s_drawTileset != TILESET_MONS32) {
                IconToBitmap(
                    m_objectIcons[s_drawTileset],
                    gWindowManager->m_screen,
                    cellPixelX,
                    cellPixelY,
                    drawnCell->m_objectIndex,
                    ICON_DRAW_OFFSET_FULL
                );
                if (drawnCell->m_flags & MAP_CELL_OBJECT_ANIMATED)
                    IconToBitmap(
                        m_objectIcons[s_drawTileset],
                        gWindowManager->m_screen,
                        cellPixelX,
                        cellPixelY,
                        drawnCell->m_objectIndex + m_animationFrame + 1,
                        ICON_DRAW_OFFSET_FULL
                    );
            }
        }
        if (drawnCell->m_flags & MAP_CELL_OBJECT_EXTRA)
            IconToBitmap(
                m_objectIcons[(drawnCell->m_objectTileset >> MAP_CELL_EXTRA_TILESET_SHIFT)],
                gWindowManager->m_screen,
                cellPixelX,
                cellPixelY,
                drawnCell->m_extraFrame,
                ICON_DRAW_OFFSET_FULL
            );
    }
    if (drawMask & ADVMGR_DRAW_HERO) {
        showHero = false;
        occupyingHero = NULL;
        if (!(drawnCell->m_flags & MAP_CELL_OBJECT_SHADOW_ONLY)
            && drawnCell->m_objectIndex != MAP_CELL_NO_FRAME) {
            s_drawTileset =
                (drawnCell->m_objectTileset & MAP_CELL_TILESET_MASK);
            if (s_drawTileset == TILESET_MONS32
                && drawnCell->m_objectIndex <= CREATURE_LAST) {
                if (mapX == m_combatMonsterX && mapY == m_combatMonsterY) {
                    if (m_combatMonsterFacingLeft)
                        FlipIconToBitmap(
                            m_objectIcons[TILESET_MINIMON],
                            gWindowManager->m_screen,
                            cellPixelX + 36,
                            cellPixelY - MONSTER_DRAW_Y_OFFSET,
                            drawnCell->m_objectIndex * MONSTER_FRAME_STRIDE
                                + MONSTER_FACING_FRAME_BASE,
                            ICON_DRAW_OFFSET_FULL
                        );
                    else
                        IconToBitmap(
                            m_objectIcons[TILESET_MINIMON],
                            gWindowManager->m_screen,
                            cellPixelX,
                            cellPixelY - MONSTER_DRAW_Y_OFFSET,
                            drawnCell->m_objectIndex * MONSTER_FRAME_STRIDE
                                + MONSTER_FACING_FRAME_BASE,
                            ICON_DRAW_OFFSET_FULL
                        );
                } else {
                    ClipIconToBitmap(
                        m_objectIcons[TILESET_MINIMON],
                        gWindowManager->m_screen,
                        cellPixelX,
                        cellPixelY - MONSTER_DRAW_Y_OFFSET,
                        drawnCell->m_objectIndex * MONSTER_FRAME_STRIDE
                            + m_animationPhases[mapX & (ADVMGR_ANIMATION_PHASE_COUNT - 1)],
                        ICON_DRAW_OFFSET_FULL,
                        0,
                        0,
                        ADVENTURE_VIEWPORT_EXTENT,
                        ADVENTURE_VIEWPORT_EXTENT
                    );
                }
            }
        }
        if (drawnCell->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_SHIP)) {
            playerColor = PLAYER_COLOR_NONE;
            heroIcon = ADVMGR_HERO_ICON_BOAT;
            heroFrame = GetCursorBaseFrame(gGame->m_boats[drawnCell->m_objectMetadata].direction);
            showHero = true;
            heroYOffset = HERO_BOAT_Y_OFFSET;
        } else {
            heroYOffset = 0;
            if (drawnCell->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_HERO)) {
                occupyingHero = gGame->GetHero(drawnCell->m_objectMetadata);
                // A hero without an owner has no flag; the original read the
                // colour from before the player table (and drew a flag icon
                // from wherever it pointed).
                playerColor = (occupyingHero->IsEmbarked() || occupyingHero->m_owner < 0
                                  ? PLAYER_COLOR_NONE
                                  : (gGame->m_players[occupyingHero->m_owner].m_color));
                heroIcon = occupyingHero->IsEmbarked() ? static_cast<i8>(ADVMGR_HERO_ICON_BOAT)
                                                       : occupyingHero->m_heroClass;
                heroFrame = GetCursorBaseFrame(occupyingHero->m_direction);
                showHero = true;
                if (occupyingHero->IsEmbarked())
                    heroYOffset = HERO_BOAT_Y_OFFSET;
            }
        }
        if (showHero) {
            if (heroFrame & HERO_FRAME_MIRROR_FLAG) {
                if (screenX == 0 || screenY <= 1 || screenX == ADVMGR_VIEW_CELL_COUNT - 1
                    || screenY == ADVMGR_VIEW_CELL_COUNT - 1) {
                    FlipClippedIconToBitmap(
                        m_heroIcons[heroIcon],
                        gWindowManager->m_screen,
                        cellPixelX + CELL_PIXELS,
                        cellPixelY + CELL_PIXELS - 1 + heroYOffset,
                        heroFrame & HERO_FRAME_INDEX_MASK,
                        ICON_DRAW_OFFSET_FULL
                    );
                    if (playerColor != PLAYER_COLOR_NONE)
                        FlipClippedIconToBitmap(
                            m_flagIcons[playerColor],
                            gWindowManager->m_screen,
                            cellPixelX + CELL_PIXELS,
                            cellPixelY + CELL_PIXELS - 1 + heroYOffset,
                            heroFrame & HERO_FRAME_INDEX_MASK,
                            ICON_DRAW_OFFSET_FULL
                        );
                } else {
                    if (m_drawHeroShadows && heroIcon != ADVMGR_HERO_ICON_BOAT)
                        FlipDimIconToBitmap(
                            m_shadowIcon,
                            gWindowManager->m_screen,
                            cellPixelX + CELL_PIXELS,
                            cellPixelY + CELL_LAST_PIXEL,
                            heroFrame & HERO_FRAME_INDEX_MASK,
                            ICON_DRAW_OFFSET_FULL
                        );
                    FlipIconToBitmap(
                        m_heroIcons[heroIcon],
                        gWindowManager->m_screen,
                        cellPixelX + CELL_PIXELS,
                        cellPixelY + CELL_PIXELS - 1 + heroYOffset,
                        heroFrame & HERO_FRAME_INDEX_MASK,
                        ICON_DRAW_OFFSET_FULL
                    );
                    if (playerColor != PLAYER_COLOR_NONE)
                        FlipIconToBitmap(
                            m_flagIcons[playerColor],
                            gWindowManager->m_screen,
                            cellPixelX + CELL_PIXELS,
                            cellPixelY + CELL_PIXELS - 1 + heroYOffset,
                            heroFrame & HERO_FRAME_INDEX_MASK,
                            ICON_DRAW_OFFSET_FULL
                        );
                }
            } else if (screenX == 0 || screenY <= 1 || screenX == ADVMGR_VIEW_CELL_COUNT - 1
                       || screenY == ADVMGR_VIEW_CELL_COUNT - 1) {
                ClippedIconToBitmap(
                    m_heroIcons[heroIcon],
                    gWindowManager->m_screen,
                    cellPixelX,
                    cellPixelY + CELL_PIXELS - 1 + heroYOffset,
                    heroFrame,
                    ICON_DRAW_OFFSET_FULL
                );
                if (playerColor != PLAYER_COLOR_NONE)
                    ClippedIconToBitmap(
                        m_flagIcons[playerColor],
                        gWindowManager->m_screen,
                        cellPixelX,
                        cellPixelY + CELL_PIXELS - 1 + heroYOffset,
                        heroFrame & HERO_FRAME_INDEX_MASK,
                        ICON_DRAW_OFFSET_FULL
                    );
            } else {
                if (m_drawHeroShadows && heroIcon != ADVMGR_HERO_ICON_BOAT)
                    DimIconToBitmap(
                        m_shadowIcon,
                        gWindowManager->m_screen,
                        cellPixelX,
                        cellPixelY + CELL_LAST_PIXEL,
                        heroFrame,
                        ICON_DRAW_OFFSET_FULL
                    );
                IconToBitmap(
                    m_heroIcons[heroIcon],
                    gWindowManager->m_screen,
                    cellPixelX,
                    cellPixelY + CELL_PIXELS - 1 + heroYOffset,
                    heroFrame,
                    ICON_DRAW_OFFSET_FULL
                );
                if (playerColor != PLAYER_COLOR_NONE)
                    IconToBitmap(
                        m_flagIcons[playerColor],
                        gWindowManager->m_screen,
                        cellPixelX,
                        cellPixelY + CELL_PIXELS - 1 + heroYOffset,
                        heroFrame & HERO_FRAME_INDEX_MASK,
                        ICON_DRAW_OFFSET_FULL
                    );
            }
        }
        if (m_cursorActive && (drawnCell->m_flags & MAP_CELL_HERO_CURSOR) && !m_comboHeroDrawn
            && mapX == m_mapOriginX + ADVMGR_VIEW_CENTER
            && mapY == m_mapOriginY + ADVMGR_VIEW_CENTER) {
            DrawCursor();
            m_comboHeroDrawn = true;
        }
    }
    if (drawMask & ADVMGR_DRAW_OVERLAY) {
        if (drawnCell->m_overlayIndex != MAP_CELL_NO_FRAME) {
            s_drawTileset =
                (drawnCell->m_overlayTileset & MAP_CELL_TILESET_MASK);
            IconToBitmap(
                m_objectIcons[s_drawTileset],
                gWindowManager->m_screen,
                cellPixelX,
                cellPixelY,
                drawnCell->m_overlayIndex,
                ICON_DRAW_OFFSET_FULL
            );
            if (drawnCell->m_flags & MAP_CELL_OVERLAY_ANIMATED)
                IconToBitmap(
                    m_objectIcons[s_drawTileset],
                    gWindowManager->m_screen,
                    cellPixelX,
                    cellPixelY,
                    drawnCell->m_overlayIndex + m_animationFrame + 1,
                    ICON_DRAW_OFFSET_FULL
                );
        }
        if (drawnCell->m_flags & MAP_CELL_OVERLAY_EXTRA)
            IconToBitmap(
                m_objectIcons[(drawnCell->m_overlayTileset >> MAP_CELL_EXTRA_TILESET_SHIFT)],
                gWindowManager->m_screen,
                cellPixelX,
                cellPixelY,
                drawnCell->m_extraFrame,
                ICON_DRAW_OFFSET_FULL
            );
    }
}

mapCell* advManager::GetCell(i16 x, i16 y) {
    if (x < 0 || y < 0 || x >= MAP_CELL_GRID_SIZE || y >= MAP_CELL_GRID_SIZE)
        return m_mapData[0];
    else
        return &m_mapData[x][y];
}

void advManager::UpdateRadar(b8 updateScreen, b32 partial) {
    i16 y;
    i32 minX;
    i32 maxY;
    i16 x;
    i16 color;
    i16 theOwner;
    i32 maxX;
    i32 minY;
    mapCell* cellPtrItem;

    if (!partial) {
        minX = 0;
        minY = 0;
        maxX = MAP_CELL_GRID_SIZE - 1;
        maxY = MAP_CELL_GRID_SIZE - 1;
    } else {
        minX = m_mapOriginX - 1;
        minY = m_mapOriginY - 1;
        maxX = m_mapOriginX + ADVMGR_VIEW_CELL_COUNT;
        maxY = m_mapOriginY + ADVMGR_VIEW_CELL_COUNT;
        if (minX < 0)
            minX = 0;
        if (minY < 0)
            minY = 0;
        if (maxX > MAP_CELL_GRID_SIZE - 1)
            maxX = MAP_CELL_GRID_SIZE - 1;
        if (maxY > MAP_CELL_GRID_SIZE - 1)
            maxY = MAP_CELL_GRID_SIZE - 1;
    }

    if (!gThisNetHumanPlayer[gCurPlayer])
        return;

    gAdvManager->m_heroesLogoShown = false;
    for (x = minX; x <= maxX; x++) {
        for (y = minY; y <= maxY; y++) {
            if (!(gGame->m_mapExtra[x][y] & gCurPlayerBit)) {
                m_radarIcon->FillToBuffer(
                    x * RADAR_CELL_PIXELS + RADAR_LEFT,
                    y * RADAR_CELL_PIXELS + RADAR_TOP,
                    0,
                    RADAR_UNEXPLORED_COLOR,
                    ICON_DRAW_NORMAL,
                    ICON_DRAW_OFFSET_FULL
                );
                continue;
            }
            cellPtrItem = &m_mapData[x][y];
            // A hero cell without its hero is drawn as what lies there.
            if (MAP_TRIGGER_OBJECT(cellPtrItem->m_triggerType) == MAP_OBJECT_HERO
                && gGame->CellHasRecord(x, y)) {
                theOwner = gGame->m_availableHeroes[cellPtrItem->m_objectMetadata];
                if (theOwner == gCurPlayer)
                    color = gRadarOwnerColor
                        [theOwner < 0
                             ? PLAYER_COLOR_NEUTRAL
                             : (gGame->m_players[theOwner].m_color)];
                else
                    color = gRadarTerrainColor[CELL_TERRAIN(cellPtrItem)];
            } else {
                switch (
                    (cellPtrItem->m_objectTileset & MAP_CELL_TILESET_MASK)
                ) {
                    case TILESET_TOWN32:
                        theOwner = gGame->m_townOwners[cellPtrItem->m_objectMetadata];
                        color = gRadarOwnerColor
                            [theOwner < 0
                                 ? PLAYER_COLOR_NEUTRAL
                                 : (gGame->m_players[theOwner].m_color)];
                        break;
                    case TILESET_RSRC32:
                        switch (cellPtrItem->m_triggerType) {
                            case MAP_OBJECT_TRIGGER(MAP_OBJECT_ALCHEMIST_LAB):
                            case MAP_OBJECT_TRIGGER(MAP_OBJECT_MINE):
                            case MAP_OBJECT_TRIGGER(MAP_OBJECT_SAWMILL):
                            case MAP_EVENT_TRIGGER(MAP_OBJECT_ALCHEMIST_LAB):
                            case MAP_EVENT_TRIGGER(MAP_OBJECT_MINE):
                            case MAP_EVENT_TRIGGER(MAP_OBJECT_SAWMILL):
                                theOwner = gGame->m_mineOwners[cellPtrItem->m_objectMetadata];
                                color = gRadarOwnerColor
                                    [theOwner < 0
                                         ? PLAYER_COLOR_NEUTRAL
                                         : (gGame->m_players[theOwner].m_color)];
                                break;
                            default:
                                color = gRadarTerrainColor[CELL_TERRAIN(cellPtrItem)];
                                break;
                        }
                        break;
                    case TILESET_MTN32:
                    case TILESET_TREE32:
                        color = gRadarTerrainColor[CELL_TERRAIN(cellPtrItem)] + RADAR_TERRAIN_SHADE;
                        break;
                    default:
                        color = gRadarTerrainColor[CELL_TERRAIN(cellPtrItem)];
                        break;
                }
            }
            m_radarIcon->FillToBuffer(
                x * RADAR_CELL_PIXELS + RADAR_LEFT,
                y * RADAR_CELL_PIXELS + RADAR_TOP,
                0,
                color,
                ICON_DRAW_NORMAL,
                ICON_DRAW_OFFSET_FULL
            );
        }
    }
    m_radarIcon->ClipFillToBuffer(
        m_mapOriginX * RADAR_CELL_PIXELS + RADAR_LEFT,
        m_mapOriginY * RADAR_CELL_PIXELS + RADAR_TOP,
        1,
        RADAR_VIEWPORT_COLOR,
        ICON_DRAW_NORMAL,
        ICON_DRAW_OFFSET_FULL,
        RADAR_LEFT,
        RADAR_TOP,
        RADAR_SIZE,
        RADAR_SIZE
    );
    if (updateScreen)
        gWindowManager->UpdateScreenRegion(
            minX * RADAR_CELL_PIXELS + RADAR_LEFT,
            minY * RADAR_CELL_PIXELS + RADAR_TOP,
            (maxX - minX + 1) * RADAR_CELL_PIXELS,
            (maxY - minY + 1) * RADAR_CELL_PIXELS
        );
}

// Whether the current hero (or, for an obelisk, the current player) has
// already used the object on this cell.
static i32 SiteVisited(mapCell* cell) {
    hero* currentHero;

    if ((cell->m_triggerType & MAP_TRIGGER_TYPE_MASK) == MAP_OBJECT_OBELISK)
        return gGame->m_obeliskVisitors[cell->m_objectMetadata - 1] & gCurPlayerBit;
    if (gCurPlayerData->m_currentHero == HERO_ID_NONE)
        return 0;
    currentHero = gGame->GetHero(gCurPlayerData->m_currentHero);
    switch (cell->m_triggerType & MAP_TRIGGER_TYPE_MASK) {
        case MAP_OBJECT_BUOY:
            return currentHero->m_eventFlags & HERO_EVENT_BUOY;
        case MAP_OBJECT_FAERIE_RING:
            return currentHero->m_eventFlags & HERO_EVENT_FAERIE_RING;
        case MAP_OBJECT_FOUNTAIN:
            return currentHero->m_eventFlags & HERO_EVENT_FOUNTAIN;
        case MAP_OBJECT_OASIS:
            return currentHero->m_eventFlags & HERO_EVENT_OASIS;
        case MAP_OBJECT_STATUE:
            return currentHero->m_eventFlags & HERO_EVENT_STATUE;
        case MAP_OBJECT_GAZEBO:
            return currentHero->m_visitedSites & (1 << (cell->m_objectMetadata & 31));
    }
    return 0;
}

// The name of an object type for the quick info. The table has 63 entries,
// but map cells carry types up to 127; the original read on into the tables
// the linker placed after it (the 36 town names, then the event texts), and
// so showed those. The same text is chosen here without leaving the tables.
static char* QuickInfoObjectName(i32 type) {
    if (type < 63)
        return gObjectNames[type];
    if (type < 63 + GAME_TOWN_COUNT)
        return gTownNames[type - 63];
    return gEventText[type - 63 - GAME_TOWN_COUNT];
}

void advManager::QuickInfo(i16 cellX, i16 cellY) {
    mapCell* currentCell;
    i16 posX;
    heroWindow* pWin;
    i16 posY;
    char savedTextLocal[200];
    tag_message message;
    i16 quickInfoShowFlag;

    quickInfoShowFlag = 1;
    currentCell = NULL;
    posX = cellX * CELL_PIXELS - QUICK_INFO_X_OFFSET;
    if (posX < BORDER_EDGE_SIZE)
        posX = BORDER_EDGE_SIZE;
    if (posX + QUICK_INFO_WIDTH > BORDER_MIDDLE_END)
        posX = QUICK_INFO_RIGHT_X;
    posY = cellY * CELL_PIXELS - QUICK_INFO_Y_OFFSET;
    if (posY < BORDER_EDGE_SIZE)
        posY = BORDER_EDGE_SIZE;
    if (posY + QUICK_INFO_HEIGHT > BORDER_MIDDLE_END)
        posY = QUICK_INFO_BOTTOM_Y;

    pWin = new heroWindow(posX, posY, "qwikinfo.bin");
    if (!pWin)
        MemError();

    if (m_mapOriginX + cellX < 0 || m_mapOriginX + cellX >= MAP_CELL_GRID_SIZE
        || m_mapOriginY + cellY < 0 || m_mapOriginY + cellY >= MAP_CELL_GRID_SIZE) {
        sprintf(gText, "\n\n%s", localization::Tr("adventure.quick_info.border"));
    } else {
        currentCell = GetCell(m_mapOriginX + cellX, m_mapOriginY + cellY);
        if (!(gGame->m_mapExtra[m_mapOriginX + cellX][m_mapOriginY + cellY] & gCurPlayerBit)) {
            sprintf(gText, "\n\n%s", localization::Tr("adventure.quick_info.uncharted"));
        } else {
            switch (MAP_TRIGGER_OBJECT(currentCell->m_triggerType)) {
                case MAP_OBJECT_ARTIFACT:
                    sprintf(gText, "\n\n%s", localization::Tr("adventure.quick_info.artifact"));
                    break;
                case MAP_OBJECT_NONE:
                case MAP_OBJECT_COAST:
                case MAP_OBJECT_SHADOW:
                    sprintf(gText, "\n\n%s", gTerrainNames[CELL_TERRAIN(currentCell)]);
                    break;
                case MAP_OBJECT_MINE:
                    sprintf(
                        gText,
                        "\n\n%s",
                        gMineNames[gGame->m_mines[currentCell->m_objectMetadata].type]
                    );
                    break;
                // A resource or monster cell whose picture is not one of
                // theirs (some shipped maps have such cells) indexed the name
                // tables far past their end; it gets the object type's name.
                case MAP_OBJECT_RESOURCE:
                    if (currentCell->m_objectIndex < RESOURCE_PILE_OBJECT_BASE
                        || currentCell->m_objectIndex >= RESOURCE_PILE_OBJECT_BASE + RESOURCE_COUNT) {
                        sprintf(gText, "\n\n%s", QuickInfoObjectName(MAP_OBJECT_RESOURCE));
                        break;
                    }
                    sprintf(
                        gText,
                        "\n\n%s",
                        gResourceNames[(currentCell->m_objectIndex - RESOURCE_PILE_OBJECT_BASE)]
                    );
                    break;
                case MAP_OBJECT_RESOURCE_SHADOW:
                    if (currentCell->m_objectIndex + 2 >= RESOURCE_COUNT) {
                        sprintf(gText, "\n\n%s", QuickInfoObjectName(MAP_OBJECT_RESOURCE));
                        break;
                    }
                    sprintf(gText, "\n\n%s", gResourceNames[(currentCell->m_objectIndex + 2)]);
                    break;
                case MAP_OBJECT_MONSTER:
                    if (currentCell->m_objectIndex >= CREATURE_COUNT) {
                        sprintf(gText, "\n\n%s", QuickInfoObjectName(MAP_OBJECT_MONSTER));
                        break;
                    }
                    sprintf(
                        gText,
                        "\n\n%s %s",
                        GetArmySizeName(
                            currentCell->m_objectMetadata & MONSTER_COUNT_MASK,
                            ARMY_SIZE_NAME_SENTENCE
                        ),
                        gArmyNamesPlural[currentCell->m_objectIndex]
                    );
                    break;
                default:
                    sprintf(
                        gText,
                        SiteVisited(currentCell)
                            ? localization::Tr("adventure.quick_info.visited")
                            : "\n\n%s",
                        QuickInfoObjectName(currentCell->m_triggerType & MAP_TRIGGER_TYPE_MASK)
                    );
                    break;
            }
        }
    }

    strcpy(savedTextLocal, gText);
    if (gDebugLevel > 0 && currentCell)
        sprintf(
            gText,
            "otile%d oi%d ot%d ei%d fl%d %s X%d Y%d",
            currentCell->m_objectTileset,
            currentCell->m_objectIndex,
            currentCell->m_triggerType,
            currentCell->m_objectMetadata,
            currentCell->m_flags & MAP_CELL_OBJECT_SHADOW_ONLY,
            savedTextLocal,
            m_mapOriginX + cellX,
            m_mapOriginY + cellY
        );
    SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_TEXT, 1);
    message.text = gText;
    pWin->BroadcastMessage(message);
    GrabScreen();
    gWindowManager->AddWindow(pWin, WINDOW_Z_ORDER_APPEND, 1);
    gMouseManager->HideSystemCursor();
    QuickViewWait();
    gWindowManager->RemoveWindow(pWin);
    delete pWin;
    gMouseManager->ShowSystemCursor();
}

void advManager::UpdateHeroLocator(i32 locatorSlot, b8 drawWindow, b8 updateScreen) {
    i32 widgetBase;
    i8 whichHero;
    i32 curHero;
    tag_message message;
    i32 i;
    hero* heroPtr;
    i32 moveFrm;

    if (!gThisNetHumanPlayer[gCurPlayer])
        return;
    if (locatorSlot == LOCATOR_SLOT_CURRENT_HERO) {
        curHero = gCurPlayerData->CurrentHero();
        if (curHero == HERO_ID_NONE)
            return;
        for (i = 0; i < LOCATOR_VISIBLE_COUNT; i++) {
            if (curHero == gCurPlayerData->m_heroIds[gCurPlayerData->m_heroLocatorPage + i])
                locatorSlot = i;
        }
        if (locatorSlot == LOCATOR_SLOT_CURRENT_HERO)
            return;
    }
    widgetBase = locatorSlot * HERO_LOCATOR_WIDGET_STRIDE + HERO_LOCATOR_WIDGET_BASE;
    message.type = MESSAGE_WIDGET;
    whichHero = gCurPlayerData->m_heroIds[gCurPlayerData->m_heroLocatorPage + locatorSlot];
    message.command = WIDGET_COMMAND_SET_COLOR;
    message.id = widgetBase + HERO_LOCATOR_HIGHLIGHT;
    message.value = (whichHero == gCurPlayerData->m_currentHero
                     && gCurPlayerData->m_currentHero != HERO_ID_NONE && !gAllBlack)
                        ? LOCATOR_HIGHLIGHT_COLOR
                        : 0;
    m_adventureWindow->BroadcastMessage(message);
    if (whichHero == HERO_ID_NONE || gAllBlack) {
        message.id = widgetBase + HERO_LOCATOR_BUTTON;
        message.command = WIDGET_COMMAND_SET_FRAME;
        message.value = locatorSlot;
        m_adventureWindow->BroadcastMessage(message);
        message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.value = WIDGET_FLAG_DRAW;
        for (i = 0; i <= HERO_LOCATOR_BUTTON - 1; i++) {
            message.id = widgetBase + i;
            m_adventureWindow->BroadcastMessage(message);
        }
    } else {
        heroPtr = gGame->GetHero(whichHero);
        message.id = widgetBase + HERO_LOCATOR_BUTTON;
        message.command = WIDGET_COMMAND_SET_FRAME;
        message.value = LOCATOR_FRAME_HERO;
        m_adventureWindow->BroadcastMessage(message);
        message.command = WIDGET_COMMAND_SET_FLAGS;
        message.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
        for (i = 0; i <= HERO_LOCATOR_HIGHLIGHT; i++) {
            message.id = widgetBase + i;
            m_adventureWindow->BroadcastMessage(message);
        }
        moveFrm = heroPtr->m_remainingMobility * 22 / 60;
        if (moveFrm < 0)
            moveFrm = 0;
        if (moveFrm > 30)
            moveFrm = 25;
        else if (moveFrm > 26)
            moveFrm = 24;
        else if (moveFrm > 23)
            moveFrm = 23;
        message.id = widgetBase + HERO_LOCATOR_MOBILITY;
        message.command = WIDGET_COMMAND_SET_FRAME;
        message.value = moveFrm;
        m_adventureWindow->BroadcastMessage(message);
        message.id = widgetBase + HERO_LOCATOR_PORTRAIT;
        message.command = WIDGET_COMMAND_SET_FRAME;
        message.value = whichHero;
        m_adventureWindow->BroadcastMessage(message);
        message.id = widgetBase + 3;
        message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
        m_adventureWindow->BroadcastMessage(message);
        message.id = widgetBase + 4;
        message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
        m_adventureWindow->BroadcastMessage(message);
    }
    if (drawWindow) {
        m_adventureWindow->DrawWindow(0, widgetBase, widgetBase + HERO_LOCATOR_HIGHLIGHT);
        if (updateScreen)
            gWindowManager->UpdateScreenRegion(481, locatorSlot * 32 + 177, 54, 30);
    }
}

void advManager::UpdateHeroLocators(b8 drawWindow, i8 updateScreen) {
    i32 locatorSlot;
    double scrollStep;

    if (!gThisNetHumanPlayer[gCurPlayer])
        return;

    for (locatorSlot = 0; locatorSlot < LOCATOR_VISIBLE_COUNT; ++locatorSlot)
        UpdateHeroLocator(locatorSlot, false, false);

    if (gCurPlayerData->m_heroCount < LOCATOR_PAGE_THRESHOLD) {
        m_scrollLeftButton->m_y = LOCATOR_SCROLL_NO_PAGES_Y;
    } else {
        scrollStep = 73.0 / (gCurPlayerData->m_heroCount - LOCATOR_PAGE_DENOMINATOR_OFFSET);
        m_scrollLeftButton->m_y = gCurPlayerData->m_heroLocatorPage * scrollStep + 195.0;
    }
    if (drawWindow)
        m_adventureWindow->DrawWindow(updateScreen);
}

void advManager::UpdateTownLocators(b8 drawWindow, i8 updateScreen) {
    i16 i;
    i8 whichTown;
    tag_message msg;
    double step;

    if (!gThisNetHumanPlayer[gCurPlayer])
        return;
    msg.type = MESSAGE_WIDGET;
    for (i = 0; i < LOCATOR_VISIBLE_COUNT; i++) {
        whichTown = gCurPlayerData->m_townIds[gCurPlayerData->m_townLocatorPage + i];
        msg.command = WIDGET_COMMAND_SET_COLOR;
        msg.id = i + TOWN_LOCATOR_HIGHLIGHT_FIRST;
        msg.value = (gCurPlayerData->m_currentTown != GAME_TOWN_NONE
                     && whichTown == gCurPlayerData->m_currentTown && !gAllBlack)
                        ? LOCATOR_HIGHLIGHT_COLOR
                        : 0;
        m_adventureWindow->BroadcastMessage(msg);
        msg.id = i + ADVENTURE_CONTROL_TOWN_LOCATOR_1;
        if (whichTown == GAME_TOWN_NONE || gAllBlack) {
            msg.command = WIDGET_COMMAND_SET_FRAME;
            msg.value = i + LOCATOR_FRAME_EMPTY_TOWN_FIRST;
            m_adventureWindow->BroadcastMessage(msg);
            msg.command = WIDGET_COMMAND_CLEAR_FLAGS;
            msg.value = WIDGET_FLAG_ENABLED;
            m_adventureWindow->BroadcastMessage(msg);
        } else {
            msg.command = WIDGET_COMMAND_SET_FLAGS;
            msg.value = WIDGET_FLAG_ENABLED;
            m_adventureWindow->BroadcastMessage(msg);
            msg.command = WIDGET_COMMAND_SET_FRAME;
            msg.value = (gGame->GetTown(whichTown)->m_type)
                        + LOCATOR_FRAME_TOWN_FIRST;
            if (gGame->GetTown(whichTown)->m_buildings
                & (1 << BUILDING_SLOT_CASTLE))
                msg.value += LOCATOR_FRAME_CASTLE_OFFSET;
            m_adventureWindow->BroadcastMessage(msg);
        }
    }
    if (gCurPlayerData->m_townCount < LOCATOR_PAGE_THRESHOLD) {
        m_scrollRightButton->m_y = LOCATOR_SCROLL_NO_PAGES_Y;
    } else {
        step = 74.0 / (gCurPlayerData->m_townCount - LOCATOR_PAGE_DENOMINATOR_OFFSET);
        m_scrollRightButton->m_y = gCurPlayerData->m_townLocatorPage * step + 195.0;
    }
    if (drawWindow)
        m_adventureWindow->DrawWindow(updateScreen);
}

void advManager::UpdBottomView(b8 forceUpdate, b8 drawWindow, b8 updateScreen) {
    b8 updated;

    updated = false;
    gForceUpdate = forceUpdate;
    if (gBottomViewOverride == BOTTOM_VIEW_OVERRIDE_DISABLED)
        return;

    if (gBottomViewOverride > BOTTOM_VIEW_NONE) {
        if (KBTickCount() > gBottomViewOverrideEndTime) {
            gBottomViewOverride = BOTTOM_VIEW_NONE;
        } else {
            switch (gBottomViewOverride) {
                case BOTTOM_VIEW_NEW_TURN:
                    updated = UpdBottomViewNewTurn();
                    break;
                case BOTTOM_VIEW_KINGDOM:
                    updated = UpdBottomViewKingdom();
                    break;
                case BOTTOM_VIEW_RESOURCE:
                    updated = UpdBottomViewResMsg();
                    break;
            }
            goto update_bottom_view;
        }
    }

    if (!gThisNetHumanPlayer[gCurPlayer] || gAllBlack)
        updated = UpdBottomViewEnemyTurn();
    else if (gCurPlayerData->CurrentHero() == HERO_ID_NONE)
        updated = UpdBottomViewKingdom();
    else
        updated = UpdBottomViewHero();

update_bottom_view:
    if (updated && drawWindow) {
        m_adventureWindow
            ->DrawWindow(0, BOTTOM_VIEW_DRAW_FIRST_WIDGET, BOTTOM_VIEW_DRAW_LAST_WIDGET);
        if (updateScreen)
            gWindowManager->UpdateScreenRegion(
                BOTTOM_VIEW_PANEL_X,
                BOTTOM_VIEW_PANEL_Y,
                BOTTOM_VIEW_PANEL_WIDTH,
                BOTTOM_VIEW_PANEL_HEIGHT
            );
    }
    forceUpdate = gForceUpdate;
}

void advManager::ClearBottomView(void) {
    i32 widgetIndex;

    if (gCurBottomView == BOTTOM_VIEW_NONE)
        return;

    for (widgetIndex = 0; widgetIndex < ADVMGR_BOTTOM_VIEW_WIDGET_COUNT; ++widgetIndex) {
        if (m_bottomViewPrimaryWidgets[widgetIndex] != NULL) {
            m_adventureWindow->RemoveWidget(m_bottomViewPrimaryWidgets[widgetIndex]);
            delete m_bottomViewPrimaryWidgets[widgetIndex];
        }
        if (m_bottomViewSecondaryWidgets[widgetIndex] != NULL) {
            m_adventureWindow->RemoveWidget(m_bottomViewSecondaryWidgets[widgetIndex]);
            delete m_bottomViewSecondaryWidgets[widgetIndex];
        }
        m_bottomViewPrimaryWidgets[widgetIndex] = NULL;
        m_bottomViewSecondaryWidgets[widgetIndex] = NULL;
    }
    gCurBottomViewEnemy = BOTTOM_VIEW_NO_ENEMY;
    gCurBottomView = BOTTOM_VIEW_NONE;
    gLastAnimFrame = BOTTOM_VIEW_NO_ANIMATION;
}

b8 advManager::UpdBottomViewEnemyTurn(void) {
    static i32 gLastSandAnimTime = 0;
    static i32 gLastNewSandAnimTime = 0;
    b8 updated;
    tag_message msg;

    updated = false;
    msg.type = MESSAGE_WIDGET;
    if (gCurBottomView != BOTTOM_VIEW_ENEMY_TURN) {
        updated = true;
        gForceUpdate = true;
        ClearBottomView();
        gCurBottomView = BOTTOM_VIEW_ENEMY_TURN;

        m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_BACKGROUND] = new iconWidget(
            BOTTOM_VIEW_PANEL_X,
            BOTTOM_VIEW_PANEL_Y,
            BOTTOM_VIEW_PANEL_WIDTH,
            BOTTOM_VIEW_PANEL_HEIGHT,
            "stonback.icn",
            0,
            ICON_DRAW_NORMAL,
            BOTTOM_VIEW_BACKGROUND_ID,
            ICON_WIDGET_DRAW,
            1
        );
        if (!m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_BACKGROUND])
            MemError();
        m_adventureWindow->AddWidget(
            m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_BACKGROUND],
            ENEMY_TURN_BACKGROUND_Z
        );

        m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_FOREGROUND] = new iconWidget(
            ENEMY_TURN_HOURGLASS_X,
            ENEMY_TURN_HOURGLASS_Y,
            ENEMY_TURN_HOURGLASS_WIDTH,
            ENEMY_TURN_HOURGLASS_HEIGHT,
            "hourglas.icn",
            0,
            ICON_DRAW_NORMAL,
            BOTTOM_VIEW_FOREGROUND_ID,
            ICON_WIDGET_DRAW,
            1
        );
        if (!m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_FOREGROUND])
            MemError();
        m_adventureWindow->AddWidget(
            m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_FOREGROUND],
            ENEMY_TURN_HOURGLASS_Z
        );
    }

    if (gForceUpdate || KBTickCount() - gLastSandAnimTime > ENEMY_TURN_ANIMATION_DELAY) {
        gLastSandAnimTime = KBTickCount();
        gLastAnimFrame = m_animationFrame;
        if (KBTickCount() - gLastNewSandAnimTime > ENEMY_TURN_ANIMATION_DELAY) {
            gLastNewSandAnimTime = KBTickCount();
            gSandAnim++;
            if (gSandAnim >= ENEMY_TURN_SAND_FRAME_LIMIT)
                gSandAnim = ENEMY_TURN_SAND_RESTART_FRAME;
            updated = true;
            if (m_bottomViewPrimaryWidgets[ENEMY_TURN_SAND_SLOT + ADVMGR_BOTTOM_VIEW_ICON_FIRST]) {
                msg.command = WIDGET_COMMAND_SET_FRAME;
                msg.id = ENEMY_TURN_SAND_ID;
                msg.value = gSandAnim + ENEMY_TURN_SAND_FRAME_OFFSET;
                m_adventureWindow->BroadcastMessage(msg);
            } else {
                m_bottomViewPrimaryWidgets[ENEMY_TURN_SAND_SLOT + ADVMGR_BOTTOM_VIEW_ICON_FIRST] =
                    new iconWidget(
                        ENEMY_TURN_ANIMATION_X,
                        ENEMY_TURN_ANIMATION_Y,
                        ENEMY_TURN_ANIMATION_WIDTH,
                        ENEMY_TURN_ANIMATION_HEIGHT,
                        "hourglas.icn",
                        gSandAnim + ENEMY_TURN_SAND_FRAME_OFFSET,
                        ICON_DRAW_NORMAL,
                        ENEMY_TURN_SAND_ID,
                        ICON_WIDGET_DRAW,
                        1
                    );
                if (!m_bottomViewPrimaryWidgets
                        [ENEMY_TURN_SAND_SLOT + ADVMGR_BOTTOM_VIEW_ICON_FIRST])
                    MemError();
                m_adventureWindow->AddWidget(
                    m_bottomViewPrimaryWidgets
                        [ENEMY_TURN_SAND_SLOT + ADVMGR_BOTTOM_VIEW_ICON_FIRST],
                    ENEMY_TURN_SAND_Z
                );
            }
        }
    }

    if (gForceUpdate || gCurBottomViewEnemy != gCurPlayer) {
        updated = true;
        gCurBottomViewEnemy = gCurPlayer;
        if (gCurBottomViewEnemy != gCurPlayer)
            gCurHourGlassPhase = 0;
        if (m_bottomViewPrimaryWidgets[ENEMY_TURN_CREST_SLOT + ADVMGR_BOTTOM_VIEW_ICON_FIRST]) {
            msg.command = WIDGET_COMMAND_SET_FRAME;
            msg.id = ENEMY_TURN_CREST_ID;
            msg.value = (gGame->GetPlayerColor(gCurPlayer));
            m_adventureWindow->BroadcastMessage(msg);
        } else {
            m_bottomViewPrimaryWidgets[ENEMY_TURN_CREST_SLOT + ADVMGR_BOTTOM_VIEW_ICON_FIRST] =
                new iconWidget(
                    ENEMY_TURN_CREST_X,
                    ENEMY_TURN_ANIMATION_Y,
                    ENEMY_TURN_ANIMATION_WIDTH,
                    ENEMY_TURN_ANIMATION_HEIGHT,
                    "brcrest.icn",
                    (gGame->GetPlayerColor(gCurPlayer)),
                    ICON_DRAW_NORMAL,
                    ENEMY_TURN_CREST_ID,
                    ICON_WIDGET_DRAW,
                    1
                );
            if (!m_bottomViewPrimaryWidgets[ENEMY_TURN_CREST_SLOT + ADVMGR_BOTTOM_VIEW_ICON_FIRST])
                MemError();
            m_adventureWindow->AddWidget(
                m_bottomViewPrimaryWidgets[ENEMY_TURN_CREST_SLOT + ADVMGR_BOTTOM_VIEW_ICON_FIRST],
                ENEMY_TURN_CREST_Z
            );
        }
    }

    if (gForceUpdate || gCurHourGlassPhase < gLastHourGlassPhase || gLastHourGlassPhase < 0
        || (gCurHourGlassPhase > gLastHourGlassPhase
            && KBTickCount() - gLastHourGlassUpdateTime >= ENEMY_TURN_PHASE_DELAY)) {
        updated = true;
        gLastHourGlassPhase = gCurHourGlassPhase;
        gLastHourGlassUpdateTime = KBTickCount();
        if (m_bottomViewPrimaryWidgets[ENEMY_TURN_PHASE_SLOT + ADVMGR_BOTTOM_VIEW_ICON_FIRST]) {
            msg.command = WIDGET_COMMAND_SET_FRAME;
            msg.id = ENEMY_TURN_PHASE_ID;
            msg.value = gCurHourGlassPhase + ENEMY_TURN_PHASE_FRAME_OFFSET;
            m_adventureWindow->BroadcastMessage(msg);
        } else {
            m_bottomViewPrimaryWidgets[ENEMY_TURN_PHASE_SLOT + ADVMGR_BOTTOM_VIEW_ICON_FIRST] =
                new iconWidget(
                    ENEMY_TURN_ANIMATION_X,
                    ENEMY_TURN_ANIMATION_Y,
                    ENEMY_TURN_ANIMATION_WIDTH,
                    ENEMY_TURN_ANIMATION_HEIGHT,
                    "hourglas.icn",
                    gCurHourGlassPhase + ENEMY_TURN_PHASE_FRAME_OFFSET,
                    ICON_DRAW_NORMAL,
                    ENEMY_TURN_PHASE_ID,
                    ICON_WIDGET_DRAW,
                    1
                );
            if (!m_bottomViewPrimaryWidgets[ENEMY_TURN_PHASE_SLOT + ADVMGR_BOTTOM_VIEW_ICON_FIRST])
                MemError();
            m_adventureWindow->AddWidget(
                m_bottomViewPrimaryWidgets[ENEMY_TURN_PHASE_SLOT + ADVMGR_BOTTOM_VIEW_ICON_FIRST],
                ENEMY_TURN_PHASE_Z
            );
        }
    }
    return updated;
}

b8 advManager::UpdBottomViewNewTurn(void) {
    i32 frameIndex;
    i32 month;
    char* week;
    char* day;

    frameIndex = 0;
    if (!gForceUpdate && gCurBottomView == BOTTOM_VIEW_NEW_TURN)
        return false;

    ClearBottomView();
    gCurBottomView = BOTTOM_VIEW_NEW_TURN;
    if (gGame->m_day == 1 && (gGame->m_month != 1 || gGame->m_week != 1 || gGame->m_day != 1))
        frameIndex = gGame->m_week;

    m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_BACKGROUND] = new iconWidget(
        BOTTOM_VIEW_PANEL_X,
        BOTTOM_VIEW_PANEL_Y,
        BOTTOM_VIEW_BACKGROUND_WIDTH,
        BOTTOM_VIEW_PANEL_HEIGHT,
        "stonback.icn",
        0,
        ICON_DRAW_NORMAL,
        BOTTOM_VIEW_BACKGROUND_ID,
        ICON_WIDGET_DRAW,
        1
    );
    if (!m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_BACKGROUND])
        MemError();
    m_adventureWindow->AddWidget(
        m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_BACKGROUND],
        WINDOW_Z_ORDER_APPEND
    );

    m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_FOREGROUND] = new iconWidget(
        BOTTOM_VIEW_PANEL_X,
        BOTTOM_VIEW_PANEL_Y,
        BOTTOM_VIEW_PANEL_WIDTH,
        BOTTOM_VIEW_PANEL_HEIGHT,
        "sunmoon.icn",
        frameIndex,
        ICON_DRAW_NORMAL,
        BOTTOM_VIEW_FOREGROUND_ID,
        ICON_WIDGET_DRAW,
        1
    );
    if (!m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_FOREGROUND])
        MemError();
    m_adventureWindow->AddWidget(
        m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_FOREGROUND],
        WINDOW_Z_ORDER_APPEND
    );

    week = static_cast<char*>(malloc(BOTTOM_VIEW_TEXT_BUFFER_SIZE));
    sprintf(
        week,
        "%s: %d  %s: %d",
        localization::Tr("calendar.month.label"),
        gGame->m_month,
        localization::Tr("calendar.week.label"),
        gGame->m_week
    );
    m_bottomViewSecondaryWidgets[ADVMGR_BOTTOM_VIEW_TEXT] = new textWidget(
        NEW_TURN_DATE_TEXT_X,
        NEW_TURN_WEEK_TEXT_Y,
        NEW_TURN_DATE_TEXT_WIDTH,
        NEW_TURN_WEEK_TEXT_HEIGHT,
        week,
        "smalfont.fnt",
        1,
        BOTTOM_VIEW_TEXT_ID,
        WIDGET_KIND_TEXT
    );
    if (!m_bottomViewSecondaryWidgets[ADVMGR_BOTTOM_VIEW_TEXT])
        MemError();
    m_adventureWindow->AddWidget(
        m_bottomViewSecondaryWidgets[ADVMGR_BOTTOM_VIEW_TEXT],
        WINDOW_Z_ORDER_APPEND
    );

    // The day has a slot of its own, so the week's text is removed with it.
    day = static_cast<char*>(malloc(BOTTOM_VIEW_TEXT_BUFFER_SIZE));
    sprintf(day, "%s: %d", localization::Tr("calendar.day.label"), gGame->m_day);
    m_bottomViewSecondaryWidgets[ADVMGR_BOTTOM_VIEW_COUNT_TEXT] = new textWidget(
        NEW_TURN_DATE_TEXT_X,
        NEW_TURN_DAY_TEXT_Y,
        NEW_TURN_DATE_TEXT_WIDTH,
        NEW_TURN_DAY_TEXT_HEIGHT,
        day,
        "bigfont.fnt",
        1,
        BOTTOM_VIEW_TEXT_ID,
        WIDGET_KIND_TEXT
    );
    if (!m_bottomViewSecondaryWidgets[ADVMGR_BOTTOM_VIEW_COUNT_TEXT])
        MemError();
    m_adventureWindow->AddWidget(
        m_bottomViewSecondaryWidgets[ADVMGR_BOTTOM_VIEW_COUNT_TEXT],
        WINDOW_Z_ORDER_APPEND
    );
    return true;
}

b8 advManager::UpdBottomViewResMsg(void) {
    font* smFont;
    char* countText;
    i32 iconHeight;
    i32 lineCntNo;
    i32 textY;
    i32 iconWidth;
    char* messageText;

    if (!gForceUpdate && gCurBottomView == BOTTOM_VIEW_RESOURCE)
        return false;

    ClearBottomView();
    gCurBottomView = BOTTOM_VIEW_RESOURCE;
    m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_BACKGROUND] = new iconWidget(
        BOTTOM_VIEW_PANEL_X,
        BOTTOM_VIEW_PANEL_Y,
        BOTTOM_VIEW_BACKGROUND_WIDTH,
        BOTTOM_VIEW_PANEL_HEIGHT,
        "stonback.icn",
        0,
        ICON_DRAW_NORMAL,
        BOTTOM_VIEW_BACKGROUND_ID,
        ICON_WIDGET_DRAW,
        1
    );
    if (!m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_BACKGROUND])
        MemError();
    m_adventureWindow->AddWidget(
        m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_BACKGROUND],
        WINDOW_Z_ORDER_APPEND
    );

    textY = 0;
    if (gBottomViewResource < RESOURCE_FIRST) {
        textY = RESOURCE_VIEW_MULTILINE_HEIGHT;
        smFont = gResourceManager->GetFont("smalfont.fnt");
        lineCntNo = smFont->LineLength(gBottomViewText, BOTTOM_VIEW_PANEL_WIDTH);
        gResourceManager->Dispose(smFont);
        textY -= lineCntNo * RESOURCE_VIEW_LINE_HEIGHT;
    }
    messageText = static_cast<char*>(malloc(strlen(gBottomViewText) + 1));
    sprintf(messageText, gBottomViewText);
    m_bottomViewSecondaryWidgets[ADVMGR_BOTTOM_VIEW_TEXT] = new textWidget(
        BOTTOM_VIEW_PANEL_X,
        textY + RESOURCE_VIEW_TEXT_BASE_Y,
        BOTTOM_VIEW_PANEL_WIDTH,
        RESOURCE_VIEW_TEXT_HEIGHT,
        messageText,
        "smalfont.fnt",
        1,
        BOTTOM_VIEW_TEXT_ID,
        WIDGET_KIND_TEXT
    );
    if (!m_bottomViewSecondaryWidgets[ADVMGR_BOTTOM_VIEW_TEXT])
        MemError();
    m_adventureWindow->AddWidget(
        m_bottomViewSecondaryWidgets[ADVMGR_BOTTOM_VIEW_TEXT],
        WINDOW_Z_ORDER_APPEND
    );

    if (gBottomViewResource >= RESOURCE_FIRST) {
        if (gBottomViewResource == RESOURCE_GOLD) {
            iconWidth = RESOURCE_VIEW_GOLD_WIDTH;
            iconHeight = RESOURCE_VIEW_GOLD_HEIGHT;
        } else {
            iconWidth = RESOURCE_VIEW_ICON_WIDTH;
            iconHeight = RESOURCE_VIEW_ICON_HEIGHT;
        }
        m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_FOREGROUND] = new iconWidget(
            (BOTTOM_VIEW_PANEL_WIDTH - iconWidth) / 2 + BOTTOM_VIEW_PANEL_X,
            RESOURCE_VIEW_ICON_BOTTOM - iconHeight - RESOURCE_VIEW_ICON_BOTTOM_PADDING,
            iconWidth,
            iconHeight,
            "resource.icn",
            gBottomViewResource,
            ICON_DRAW_NORMAL,
            BOTTOM_VIEW_FOREGROUND_ID,
            ICON_WIDGET_DRAW,
            1
        );
        if (!m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_FOREGROUND])
            MemError();
        m_adventureWindow->AddWidget(
            m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_FOREGROUND],
            WINDOW_Z_ORDER_APPEND
        );

        countText = static_cast<char*>(malloc(BOTTOM_VIEW_COUNT_BUFFER_SIZE));
        sprintf(countText, "%d", gBottomViewResourceQty);
        m_bottomViewSecondaryWidgets[ADVMGR_BOTTOM_VIEW_COUNT_TEXT] = new textWidget(
            RESOURCE_VIEW_COUNT_X,
            RESOURCE_VIEW_COUNT_Y,
            RESOURCE_VIEW_COUNT_WIDTH,
            RESOURCE_VIEW_COUNT_HEIGHT,
            countText,
            "smalfont.fnt",
            1,
            BOTTOM_VIEW_TEXT_ID_2,
            WIDGET_KIND_TEXT
        );
        if (!m_bottomViewSecondaryWidgets[ADVMGR_BOTTOM_VIEW_COUNT_TEXT])
            MemError();
        m_adventureWindow->AddWidget(
            m_bottomViewSecondaryWidgets[ADVMGR_BOTTOM_VIEW_COUNT_TEXT],
            WINDOW_Z_ORDER_APPEND
        );
    }
    return true;
}

b8 advManager::UpdBottomViewKingdom(void) {
    i32 numVillage;
    i32 i;
    i32 nCastles;
    i8 rowY[KINGDOM_VIEW_ENTRY_COUNT];
    u8 textX[KINGDOM_VIEW_ENTRY_COUNT];
    char* countText[KINGDOM_VIEW_ENTRY_COUNT];

    if (!gForceUpdate && gCurBottomView == BOTTOM_VIEW_KINGDOM)
        return false;

    ClearBottomView();
    gCurBottomView = BOTTOM_VIEW_KINGDOM;
    rowY[RESOURCE_WOOD] = KINGDOM_VIEW_RESOURCE_TEXT_Y;
    rowY[RESOURCE_MERCURY] = KINGDOM_VIEW_RESOURCE_TEXT_Y;
    rowY[RESOURCE_ORE] = KINGDOM_VIEW_RESOURCE_TEXT_Y;
    rowY[RESOURCE_SULFUR] = KINGDOM_VIEW_RESOURCE_TEXT_Y;
    rowY[RESOURCE_CRYSTAL] = KINGDOM_VIEW_RESOURCE_TEXT_Y;
    rowY[RESOURCE_GEMS] = KINGDOM_VIEW_RESOURCE_TEXT_Y;
    rowY[RESOURCE_GOLD] = KINGDOM_VIEW_GOLD_TEXT_Y;
    rowY[KINGDOM_VIEW_CASTLE_ENTRY] = KINGDOM_VIEW_TOWN_TEXT_Y;
    rowY[KINGDOM_VIEW_TOWN_ENTRY] = KINGDOM_VIEW_TOWN_TEXT_Y;
    textX[RESOURCE_WOOD] = KINGDOM_VIEW_WOOD_TEXT_X;
    textX[RESOURCE_MERCURY] = KINGDOM_VIEW_MERCURY_TEXT_X;
    textX[RESOURCE_ORE] = KINGDOM_VIEW_ORE_TEXT_X;
    textX[RESOURCE_SULFUR] = KINGDOM_VIEW_SULFUR_TEXT_X;
    textX[RESOURCE_CRYSTAL] = KINGDOM_VIEW_CRYSTAL_TEXT_X;
    textX[RESOURCE_GEMS] = KINGDOM_VIEW_GEMS_TEXT_X;
    textX[RESOURCE_GOLD] = KINGDOM_VIEW_GOLD_TEXT_X;
    textX[KINGDOM_VIEW_CASTLE_ENTRY] = KINGDOM_VIEW_CASTLE_TEXT_X;
    textX[KINGDOM_VIEW_TOWN_ENTRY] = KINGDOM_VIEW_VILLAGE_TEXT_X;
    numVillage = 0;
    nCastles = 0;

    m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_BACKGROUND] = new iconWidget(
        BOTTOM_VIEW_PANEL_X,
        BOTTOM_VIEW_PANEL_Y,
        BOTTOM_VIEW_BACKGROUND_WIDTH,
        BOTTOM_VIEW_PANEL_HEIGHT,
        "stonback.icn",
        0,
        ICON_DRAW_NORMAL,
        BOTTOM_VIEW_BACKGROUND_ID,
        ICON_WIDGET_DRAW,
        1
    );
    if (!m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_BACKGROUND])
        MemError();
    m_adventureWindow->AddWidget(
        m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_BACKGROUND],
        WINDOW_Z_ORDER_APPEND
    );

    m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_FOREGROUND] = new iconWidget(
        KINGDOM_VIEW_ICON_X,
        KINGDOM_VIEW_ICON_Y,
        BOTTOM_VIEW_PANEL_WIDTH,
        BOTTOM_VIEW_PANEL_HEIGHT,
        "ressmall.icn",
        0,
        ICON_DRAW_NORMAL,
        BOTTOM_VIEW_FOREGROUND_ID,
        ICON_WIDGET_DRAW,
        1
    );
    if (!m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_FOREGROUND])
        MemError();
    m_adventureWindow->AddWidget(
        m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_FOREGROUND],
        WINDOW_Z_ORDER_APPEND
    );

    for (i = 0; i < gCurPlayerData->m_townCount; i++) {
        if (gGame->m_castleRecs[gCurPlayerData->m_townIds[i]].m_buildings
            & (1 << BUILDING_SLOT_CASTLE))
            nCastles++;
        else
            numVillage++;
    }

    for (i = 0; i < KINGDOM_VIEW_ENTRY_COUNT; i++) {
        countText[i] = static_cast<char*>(malloc(BOTTOM_VIEW_COUNT_BUFFER_SIZE));
        if (i < KINGDOM_VIEW_CASTLE_ENTRY)
            FormatAbbreviatedCount(
                countText[i],
                gCurPlayerData->m_resources[i],
                i == RESOURCE_GOLD ? KINGDOM_VIEW_GOLD_THOUSANDS : KINGDOM_VIEW_THOUSANDS
            );
        else if (i == KINGDOM_VIEW_CASTLE_ENTRY)
            sprintf(countText[i], "%d", nCastles);
        else
            sprintf(countText[i], "%d", numVillage);
        m_bottomViewSecondaryWidgets[i] = new textWidget(
            textX[i] + KINGDOM_VIEW_TEXT_X_BASE,
            rowY[i] + KINGDOM_VIEW_TEXT_Y_BASE,
            KINGDOM_VIEW_TEXT_WIDTH,
            KINGDOM_VIEW_TEXT_HEIGHT,
            countText[i],
            "smalfont.fnt",
            1,
            i + BOTTOM_VIEW_TEXT_ID,
            WIDGET_KIND_TEXT
        );
        if (!m_bottomViewSecondaryWidgets[i])
            MemError();
        m_adventureWindow->AddWidget(m_bottomViewSecondaryWidgets[i], WINDOW_Z_ORDER_APPEND);
    }
    return true;
}

// The count label's distance from the middle of the creature icon, by the
// length of the count.
static i32 gBottomHeroLabelOffset[] = {16, 16, 16, 14, 12};

b8 advManager::UpdBottomViewHero(void) {
    i32 bigCount;
    i8 creatureType;
    i16 slotNumPos;
    char* countStrData[ARMY_GROUP_SLOT_COUNT];
    i32 j;
    hero* curHero;
    i32 y;
    i32 iconDrawX;
    i16 nStacks;
    i16 iCrest;
    char* heroNameCopy;
    i32 labelDrawX;

    if (!gForceUpdate && gCurBottomView == BOTTOM_VIEW_HERO)
        return false;

    ClearBottomView();
    gCurBottomView = BOTTOM_VIEW_HERO;
    curHero = gGame->GetHero(gCurPlayerData->CurrentHero());
    nStacks = 0;

    m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_BACKGROUND] = new iconWidget(
        BOTTOM_VIEW_PANEL_X,
        BOTTOM_VIEW_PANEL_Y,
        BOTTOM_VIEW_PANEL_WIDTH,
        BOTTOM_VIEW_PANEL_HEIGHT,
        "stonback.icn",
        0,
        ICON_DRAW_NORMAL,
        BOTTOM_VIEW_BACKGROUND_ID,
        ICON_WIDGET_DRAW,
        1
    );
    if (!m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_BACKGROUND])
        MemError();
    m_adventureWindow->AddWidget(
        m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_BACKGROUND],
        WINDOW_Z_ORDER_APPEND
    );

    iCrest = (gCurPlayerData->Color()) * HERO_CLASS_COUNT
             + curHero->m_heroClass;
    m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_FOREGROUND] = new iconWidget(
        495,
        395,
        25,
        25,
        "smcrest.icn",
        iCrest,
        ICON_DRAW_NORMAL,
        BOTTOM_VIEW_FOREGROUND_ID,
        ICON_WIDGET_DRAW,
        1
    );
    if (!m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_FOREGROUND])
        MemError();
    m_adventureWindow->AddWidget(
        m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_FOREGROUND],
        WINDOW_Z_ORDER_APPEND
    );

    heroNameCopy = static_cast<char*>(malloc(9));
    strcpy(heroNameCopy, curHero->m_shortName);
    heroNameCopy[8] = 0;
    m_bottomViewSecondaryWidgets[ADVMGR_BOTTOM_VIEW_TEXT] = new textWidget(
        475,
        418,
        66,
        12,
        heroNameCopy,
        "smalfont.fnt",
        1,
        BOTTOM_VIEW_TEXT_ID,
        WIDGET_KIND_TEXT
    );
    if (!m_bottomViewSecondaryWidgets[ADVMGR_BOTTOM_VIEW_TEXT])
        MemError();
    m_adventureWindow->AddWidget(
        m_bottomViewSecondaryWidgets[ADVMGR_BOTTOM_VIEW_TEXT],
        WINDOW_Z_ORDER_APPEND
    );

    for (j = 0; j < ARMY_GROUP_SLOT_COUNT; j++) {
        if (curHero->m_army.m_creatureTypes[j] != CREATURE_NONE)
            nStacks++;
    }
    if (nStacks) {
        // The stacks are laid out from the last slot, so the panel reads in
        // army order; large counts are shown in thousands.
        slotNumPos = 0;
        for (j = ARMY_GROUP_SLOT_COUNT - 1; j >= 0; j--) {
            creatureType = curHero->m_army.m_creatureTypes[j];
            if (creatureType != CREATURE_NONE) {
                countStrData[slotNumPos] = static_cast<char*>(malloc(BOTTOM_HERO_LABEL_BYTES));
                bigCount = curHero->m_army.m_creatureCounts[j] >= BOTTOM_HERO_THOUSANDS;
                FormatAbbreviatedCount(
                    countStrData[slotNumPos],
                    curHero->m_army.m_creatureCounts[j],
                    BOTTOM_HERO_THOUSANDS
                );
                y = slotNumPos <= 2 ? 38 : 3;
                if (slotNumPos == 0) {
                    iconDrawX = nStacks <= 2 ? 81 : 97;
                } else if (slotNumPos == 1) {
                    iconDrawX = nStacks == BOTTOM_HERO_TWO_STACKS ? 32 : 52;
                } else if (slotNumPos == BOTTOM_HERO_SLOT_THIRD) {
                    iconDrawX = 7;
                } else if (slotNumPos == BOTTOM_HERO_SLOT_FOURTH) {
                    iconDrawX = nStacks == BOTTOM_HERO_FOUR_STACKS ? 81 : 97;
                } else {
                    iconDrawX = 52;
                }
                m_bottomViewPrimaryWidgets[slotNumPos + ADVMGR_BOTTOM_VIEW_ICON_FIRST] =
                    new iconWidget(
                        iconDrawX + BOTTOM_VIEW_PANEL_X,
                        y + BOTTOM_VIEW_PANEL_Y,
                        BOTTOM_HERO_ICON_WIDTH,
                        BOTTOM_HERO_ICON_HEIGHT,
                        "mons32.icn",
                        creatureType,
                        ICON_DRAW_NORMAL,
                        slotNumPos + BOTTOM_HERO_FIRST_ICON_ID,
                        ICON_WIDGET_DRAW,
                        1
                    );
                if (!m_bottomViewPrimaryWidgets[slotNumPos + ADVMGR_BOTTOM_VIEW_ICON_FIRST])
                    MemError();
                labelDrawX =
                    iconDrawX + gMons32Width[creatureType] / 2
                    + (bigCount ? BOTTOM_HERO_THOUSANDS_LABEL_OFFSET
                                : gBottomHeroLabelOffset[strlen(countStrData[slotNumPos])]);
                m_bottomViewSecondaryWidgets[slotNumPos + ADVMGR_BOTTOM_VIEW_HERO_TEXT_FIRST] =
                    new textWidget(
                        labelDrawX + BOTTOM_VIEW_PANEL_X,
                        y + 414,
                        (strlen(countStrData[slotNumPos]) + bigCount) * BOTTOM_HERO_CHARACTER_WIDTH,
                        BOTTOM_HERO_LABEL_HEIGHT,
                        countStrData[slotNumPos],
                        "smalfont.fnt",
                        1,
                        slotNumPos + BOTTOM_HERO_FIRST_TEXT_ID,
                        WIDGET_KIND_TEXT
                    );
                if (!m_bottomViewSecondaryWidgets[slotNumPos + ADVMGR_BOTTOM_VIEW_HERO_TEXT_FIRST])
                    MemError();
                m_adventureWindow->AddWidget(
                    m_bottomViewPrimaryWidgets[slotNumPos + ADVMGR_BOTTOM_VIEW_ICON_FIRST],
                    WINDOW_Z_ORDER_APPEND
                );
                m_adventureWindow->AddWidget(
                    m_bottomViewSecondaryWidgets[slotNumPos + ADVMGR_BOTTOM_VIEW_HERO_TEXT_FIRST],
                    WINDOW_Z_ORDER_APPEND
                );
                slotNumPos++;
            }
        }
    }
    return true;
}

void advManager::HeroQuickView(i8 heroId, i8 locatorSlot, i16 windowX, i16 windowY) {
    i16 creatureCount;
    i16 armyWidth;
    i16 leftEdge;
    i16 creatureY;
    i16 iconWidth;
    i16 creatureIconHeight;
    textWidget* sizeWidgets[ARMY_GROUP_SLOT_COUNT];
    i16 enableFlag;
    i16 portId;
    i16 statId;
    i16 playerColorWidget;
    heroWindow* win;
    hero* targetHero;
    char* labels[ARMY_GROUP_SLOT_COUNT];
    i16 ii;
    i16 savedY;
    i16 oldX;
    iconWidget* stackIcons[ARMY_GROUP_SLOT_COUNT];
    tag_message msg;

    armyWidth = HERO_QUICK_ARMY_AREA_WIDTH;
    leftEdge = ARMY_QUICK_AREA_LEFT;
    creatureY = HERO_QUICK_DETAILED_CREATURE_Y;
    iconWidth = ARMY_QUICK_ICON_SIZE;
    creatureIconHeight = ARMY_QUICK_ICON_SIZE;
    enableFlag = 1;
    portId = QUICK_VIEW_PORTRAIT;
    statId = QUICK_VIEW_STAT_FIRST;
    playerColorWidget = QUICK_VIEW_FLAG;
    msg.type = MESSAGE_WIDGET;
    if (heroId == HERO_ID_NONE)
        return;
    targetHero = gGame->GetHero(heroId);
    if (targetHero->m_owner == gCurPlayer || m_identifyHeroActive == true) {
        if (windowX == QUICK_VIEW_AT_LOCATOR) {
            windowX = HERO_QUICK_DEFAULT_WINDOW_X;
            windowY = locatorSlot * HERO_QUICK_LOCATOR_ROW_HEIGHT + HERO_QUICK_LOCATOR_BASE_Y;
        }
        win = new heroWindow(windowX, windowY, "qhero0.bin");
        if (!win)
            MemError();
        SetWinText(win, WINDOW_TEXT_HERO_QUICK_VIEW);
    } else {
        win = new heroWindow(windowX, windowY, "qhero1.bin");
        if (!win)
            MemError();
    }

    msg.command = WIDGET_COMMAND_SET_FRAME;
    msg.id = QUICK_VIEW_PORTRAIT;
    msg.value = targetHero->m_id;
    win->BroadcastMessage(msg);
    msg.command = WIDGET_COMMAND_SET_FRAME;
    msg.id = QUICK_VIEW_FLAG;
    msg.value = (gGame->m_players[targetHero->m_owner].Color())
                * QUICK_VIEW_FLAG_COLOR_STRIDE;
    win->BroadcastMessage(msg);
    msg.id++;
    msg.value++;
    win->BroadcastMessage(msg);
    // The name line also shows the movement points left today and the full
    // day's movement; another player's heroes show it only with the
    // ShowEnemyMobility option or under Identify Hero.
    if (gConfig.showEnemyMobility || targetHero->m_owner == gCurPlayer || m_identifyHeroActive)
        sprintf(
            gText,
            "%s: %d (%d)",
            targetHero->m_name,
            targetHero->m_remainingMobility,
            targetHero->m_mobility
        );
    else
        sprintf(gText, "%s", targetHero->m_name);
    msg.command = WIDGET_COMMAND_SET_TEXT;
    msg.id = QUICK_VIEW_NAME;
    msg.text = gText;
    win->BroadcastMessage(msg);

    creatureCount = 0;
    for (ii = 0; ii < ARMY_GROUP_SLOT_COUNT; ii++) {
        if (targetHero->m_army.m_creatureTypes[ii] != CREATURE_NONE)
            creatureCount++;
    }

    if (targetHero->m_owner == gCurPlayer || m_identifyHeroActive == true) {
        for (ii = 0; ii < HERO_PRIMARY_STAT_COUNT; ii++) {
            sprintf(gText, "%d", targetHero->m_primaryStats[ii]);
            msg.id = ii + QUICK_VIEW_STAT_FIRST;
            msg.text = gText;
            win->BroadcastMessage(msg);
        }
        if (creatureCount) {
            i16 armyStart;
            i16 idx;
            i8 monster;

            armyStart = (HERO_QUICK_ARMY_AREA_WIDTH - creatureCount * ARMY_QUICK_ICON_SIZE) / 2
                        + ARMY_QUICK_AREA_LEFT;
            idx = 0;
            for (ii = 0; ii < creatureCount; ii++) {
                while (targetHero->m_army.m_creatureTypes[idx] == CREATURE_NONE)
                    idx++;
                monster = targetHero->m_army.m_creatureTypes[idx];
                if (monster != CREATURE_NONE) {
                    stackIcons[ii] = new iconWidget(
                        armyStart + ii * ARMY_QUICK_ICON_SIZE,
                        HERO_QUICK_DETAILED_CREATURE_Y,
                        ARMY_QUICK_ICON_SIZE,
                        ARMY_QUICK_ICON_SIZE,
                        "mons32.icn",
                        monster,
                        ICON_DRAW_NORMAL,
                        WIDGET_ID_NONE,
                        ICON_WIDGET_DRAW,
                        1
                    );
                    if (!stackIcons[ii])
                        MemError();
                    labels[ii] = static_cast<char*>(malloc(HERO_QUICK_ARMY_LABEL_CAPACITY));
                    sprintf(labels[ii], "%d", targetHero->m_army.m_creatureCounts[idx]);
                    sizeWidgets[ii] = new textWidget(
                        armyStart + ii * ARMY_QUICK_ICON_SIZE,
                        HERO_QUICK_DETAILED_LABEL_Y,
                        ARMY_QUICK_ICON_SIZE,
                        ARMY_QUICK_LABEL_HEIGHT,
                        labels[ii],
                        "smalfont.fnt",
                        1,
                        WIDGET_ID_NONE,
                        WIDGET_KIND_TEXT
                    );
                    if (!sizeWidgets[ii])
                        MemError();
                    win->AddWidget(stackIcons[ii], WINDOW_Z_ORDER_APPEND);
                    win->AddWidget(sizeWidgets[ii], WINDOW_Z_ORDER_APPEND);
                }
                idx++;
            }
        }
    } else if (creatureCount) {
        i16 rowY;
        i16 topRow;
        i16 bottomRowCount;
        i8 monster;
        i16 idx;
        i16 stride;
        i16 armyStart;

        rowY = HERO_QUICK_VAGUE_FIRST_ROW_Y;
        switch (creatureCount) {
            case ARMY_QUICK_ONE_STACK:
            case ARMY_QUICK_TWO_STACKS:
            case ARMY_QUICK_THREE_STACKS:
                rowY += ARMY_QUICK_FIRST_ROW_SHIFT;
                topRow = creatureCount;
                bottomRowCount = 0;
                break;
            case ARMY_QUICK_FOUR_STACKS:
                topRow = ARMY_QUICK_FIRST_ROW_COUNT;
                bottomRowCount = 2;
                break;
            default:
                topRow = ARMY_QUICK_FIRST_ROW_COUNT;
                bottomRowCount = 3;
                break;
        }
        idx = 0;
        stride = HERO_QUICK_ARMY_AREA_WIDTH / topRow;
        armyStart = (stride - ARMY_QUICK_ICON_SIZE) / 2 + ARMY_QUICK_AREA_LEFT;
        for (ii = 0; ii < topRow; ii++) {
            while (targetHero->m_army.m_creatureTypes[idx] == CREATURE_NONE)
                idx++;
            monster = targetHero->m_army.m_creatureTypes[idx];
            stackIcons[ii] = new iconWidget(
                armyStart + stride * ii,
                rowY,
                ARMY_QUICK_ICON_SIZE,
                ARMY_QUICK_ICON_SIZE,
                "mons32.icn",
                monster,
                ICON_DRAW_NORMAL,
                WIDGET_ID_NONE,
                ICON_WIDGET_DRAW,
                1
            );
            if (!stackIcons[ii])
                MemError();
            labels[ii] = static_cast<char*>(malloc(ARMY_QUICK_SIZE_LABEL_CAPACITY));
            strcpy(
                labels[ii],
                GetArmySizeName(targetHero->m_army.m_creatureCounts[idx], ARMY_SIZE_NAME_TITLE)
            );
            sizeWidgets[ii] = new textWidget(
                stride * ii + ARMY_QUICK_AREA_LEFT,
                rowY + ARMY_QUICK_ICON_BASELINE,
                stride,
                ARMY_QUICK_LABEL_HEIGHT,
                labels[ii],
                "smalfont.fnt",
                1,
                WIDGET_ID_NONE,
                WIDGET_KIND_TEXT
            );
            if (!sizeWidgets[ii])
                MemError();
            win->AddWidget(stackIcons[ii], WINDOW_Z_ORDER_APPEND);
            win->AddWidget(sizeWidgets[ii], WINDOW_Z_ORDER_APPEND);
            idx++;
        }
        if (bottomRowCount) {
            stride = HERO_QUICK_ARMY_AREA_WIDTH / bottomRowCount;
            armyStart = (stride - ARMY_QUICK_ICON_SIZE) / 2 + ARMY_QUICK_AREA_LEFT;
            rowY += ARMY_QUICK_SECOND_ROW_SHIFT;
            for (ii = topRow; ii < topRow + bottomRowCount; ii++) {
                while (targetHero->m_army.m_creatureTypes[idx] == CREATURE_NONE)
                    idx++;
                monster = targetHero->m_army.m_creatureTypes[idx];
                stackIcons[ii] = new iconWidget(
                    armyStart + stride * (ii - ARMY_QUICK_FIRST_ROW_COUNT),
                    rowY,
                    ARMY_QUICK_ICON_SIZE,
                    ARMY_QUICK_ICON_SIZE,
                    "mons32.icn",
                    monster,
                    ICON_DRAW_NORMAL,
                    WIDGET_ID_NONE,
                    ICON_WIDGET_DRAW,
                    1
                );
                if (!stackIcons[ii])
                    MemError();
                labels[ii] = static_cast<char*>(malloc(ARMY_QUICK_SIZE_LABEL_CAPACITY));
                strcpy(
                    labels[ii],
                    GetArmySizeName(targetHero->m_army.m_creatureCounts[idx], ARMY_SIZE_NAME_TITLE)
                );
                sizeWidgets[ii] = new textWidget(
                    stride * (ii - ARMY_QUICK_FIRST_ROW_COUNT) + ARMY_QUICK_AREA_LEFT,
                    rowY + ARMY_QUICK_ICON_BASELINE,
                    stride,
                    ARMY_QUICK_LABEL_HEIGHT,
                    labels[ii],
                    "smalfont.fnt",
                    1,
                    WIDGET_ID_NONE,
                    WIDGET_KIND_TEXT
                );
                if (!sizeWidgets[ii])
                    MemError();
                win->AddWidget(stackIcons[ii], WINDOW_Z_ORDER_APPEND);
                win->AddWidget(sizeWidgets[ii], WINDOW_Z_ORDER_APPEND);
                idx++;
            }
        }
    }

    oldX = m_mapOriginX;
    savedY = m_mapOriginY;
    m_mapOriginX = targetHero->m_x - ADVMGR_VIEW_CENTER;
    m_mapOriginY = targetHero->m_y - ADVMGR_VIEW_CENTER;
    UpdateRadar(true, false);
    GrabScreen();
    gWindowManager->AddWindow(win, WINDOW_Z_ORDER_APPEND, 1);
    gMouseManager->HideSystemCursor();
    QuickViewWait();
    gWindowManager->RemoveWindow(win);
    delete win;
    gMouseManager->ShowSystemCursor();
    m_mapOriginX = oldX;
    m_mapOriginY = savedY;
    UpdateRadar(true, false);
    CompleteDraw(false);
    UpdateScreen(false, false);
    if (msg.type == MESSAGE_LEFT_BUTTON_DOWN && targetHero->m_owner == gCurPlayer)
        SetHeroContext(targetHero->m_id, false);
}

char* advManager::GetArmySizeName(i16 armySize, i8 grammar) {
    if (gDebugLevel > 0) {
        sprintf(gArmySizeName, "%d", armySize);
        return gArmySizeName;
    }
    if (armySize < static_cast<i32>(ARMY_FEW_LIMIT))
        return gArmySizeNames[static_cast<i32>(ARMY_SIZE_FEW)][static_cast<i32>(grammar)];
    if (armySize < static_cast<i32>(ARMY_SEVERAL_LIMIT))
        return gArmySizeNames[static_cast<i32>(ARMY_SIZE_SEVERAL)][static_cast<i32>(grammar)];
    if (armySize < static_cast<i32>(ARMY_PACK_LIMIT))
        return gArmySizeNames[static_cast<i32>(ARMY_SIZE_PACK)][static_cast<i32>(grammar)];
    if (armySize < static_cast<i32>(ARMY_LOTS_LIMIT))
        return gArmySizeNames[static_cast<i32>(ARMY_SIZE_LOTS)][static_cast<i32>(grammar)];
    if (armySize < static_cast<i32>(ARMY_HORDE_LIMIT))
        return gArmySizeNames[static_cast<i32>(ARMY_SIZE_HORDE)][static_cast<i32>(grammar)];
    return gArmySizeNames[static_cast<i32>(ARMY_SIZE_ZOUNDS)][static_cast<i32>(grammar)];
}

void advManager::TownQuickView(i8 townId, i8 locatorSlot, i16 windowX, i16 windowY) {
    i16 creatureCount;
    i16 creatureIconHeight;
    i16 on;
    heroWindow* window;
    town* townPtr;
    i16 playerColorWidget;
    i16 oldX;
    i32 scouting;
    i16 oldY;
    i16 armyIndex;
    i16 leftEdge;
    i16 creatureIconWidth;
    i16 faceWidget;
    tag_message message;
    i16 armyAreaWidth;

    armyAreaWidth = TOWN_QUICK_ARMY_AREA_WIDTH;
    leftEdge = ARMY_QUICK_AREA_LEFT;
    creatureIconWidth = ARMY_QUICK_ICON_SIZE;
    creatureIconHeight = ARMY_QUICK_ICON_SIZE;
    on = 1;
    faceWidget = QUICK_VIEW_PORTRAIT;
    playerColorWidget = QUICK_VIEW_FLAG;
    if (townId == GAME_TOWN_NONE)
        return;
    townPtr = gGame->GetTown(townId);
    if (windowX == QUICK_VIEW_AT_LOCATOR) {
        windowX = TOWN_QUICK_DEFAULT_WINDOW_X;
        windowY = TOWN_QUICK_DEFAULT_WINDOW_Y;
    }
    window = new heroWindow(windowX, windowY, "qtown1.bin");
    if (!window)
        MemError();
    if (townPtr->m_owner == gCurPlayer) {
        scouting = TOWN_QUICK_INFORMATION_EXACT;
    } else {
        scouting = gGame->GetNumThievesGuilds(gCurPlayer);
        if (scouting > TOWN_QUICK_INFORMATION_THIEVES_LAST)
            scouting = TOWN_QUICK_INFORMATION_THIEVES_LAST;
    }
    SetWinText(window, WINDOW_TEXT_TOWN_QUICK_VIEW);

    creatureCount = 0;
    SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_FRAME, QUICK_VIEW_PORTRAIT);
    message.value = townPtr->m_type + TOWN_QUICK_TYPE_FRAME_BASE;
    if (gGame->GetTown(townId)->m_buildings & (1 << BUILDING_SLOT_CASTLE))
        message.value += TOWN_QUICK_CASTLE_FRAME_OFFSET;
    window->BroadcastMessage(message);
    if (townPtr->m_owner == GAME_PLAYER_NONE) {
        message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.id = QUICK_VIEW_FLAG;
        message.value = WIDGET_FLAG_DRAW;
        window->BroadcastMessage(message);
        message.id++;
        window->BroadcastMessage(message);
    } else {
        message.command = WIDGET_COMMAND_SET_FRAME;
        message.id = QUICK_VIEW_FLAG;
        message.value = (gGame->m_players[townPtr->m_owner].Color())
                        * QUICK_VIEW_FLAG_COLOR_STRIDE;
        window->BroadcastMessage(message);
        message.id++;
        message.value++;
        window->BroadcastMessage(message);
    }
    sprintf(gText, GetTownName(townPtr->m_id));
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = QUICK_VIEW_NAME;
    message.text = gText;
    window->BroadcastMessage(message);

    creatureCount = 0;
    for (armyIndex = 0; armyIndex < ARMY_GROUP_SLOT_COUNT; armyIndex++) {
        if (townPtr->m_army.m_creatureTypes[armyIndex] != CREATURE_NONE)
            creatureCount++;
    }

    if (!scouting || !creatureCount) {
        char* garrisonStr;
        textWidget* garrisonWidget;

        garrisonStr = static_cast<char*>(malloc(TOWN_QUICK_EMPTY_LABEL_CAPACITY));
        if (!scouting)
            sprintf(garrisonStr, localization::Tr("adventure.quick.unknown"));
        else
            sprintf(garrisonStr, localization::Tr("adventure.quick.none"));
        garrisonWidget = new textWidget(
            TOWN_QUICK_EMPTY_LABEL_X,
            TOWN_QUICK_EMPTY_LABEL_Y,
            TOWN_QUICK_EMPTY_LABEL_WIDTH,
            ARMY_QUICK_LABEL_HEIGHT,
            garrisonStr,
            "smalfont.fnt",
            1,
            WIDGET_ID_NONE,
            WIDGET_KIND_TEXT
        );
        if (!garrisonWidget)
            MemError();
        window->AddWidget(garrisonWidget, WINDOW_Z_ORDER_APPEND);
    } else {
        i16 wIndex;
        textWidget* sizeWidgets[ARMY_GROUP_SLOT_COUNT];
        i8 monster;
        iconWidget* stackIcons[ARMY_GROUP_SLOT_COUNT];
        i16 bottomRowCount;
        i16 stride;
        b8 armySlot;
        char* troopNames[ARMY_GROUP_SLOT_COUNT];
        i32 fiveShift;
        i16 basePos;
        i16 curY;
        i8 creatureSlot;
        i16 topRow;

        curY = TOWN_QUICK_FIRST_ROW_Y;
        switch (creatureCount) {
            case ARMY_QUICK_ONE_STACK:
            case ARMY_QUICK_TWO_STACKS:
            case ARMY_QUICK_THREE_STACKS:
                curY += ARMY_QUICK_FIRST_ROW_SHIFT;
                topRow = creatureCount;
                bottomRowCount = 0;
                break;
            case ARMY_QUICK_FOUR_STACKS:
                topRow = ARMY_QUICK_FIRST_ROW_COUNT;
                bottomRowCount = 2;
                break;
            default:
                topRow = ARMY_QUICK_FIRST_ROW_COUNT;
                bottomRowCount = 3;
                break;
        }
        armySlot = false;
        wIndex = 0;
        creatureSlot = 0;
        stride = TOWN_QUICK_ARMY_AREA_WIDTH / topRow;
        basePos = (stride - ARMY_QUICK_ICON_SIZE) / 2 + ARMY_QUICK_AREA_LEFT;
        fiveShift = 0;
        for (armyIndex = 0; armyIndex < topRow; armyIndex++) {
            if (creatureCount == ARMY_GROUP_SLOT_COUNT) {
                fiveShift =
                    armyIndex == 0 ? ARMY_QUICK_FIVE_STACK_X_SHIFT : -ARMY_QUICK_FIVE_STACK_X_SHIFT;
            }
            while (townPtr->m_army.m_creatureTypes[creatureSlot] == CREATURE_NONE)
                creatureSlot++;
            monster = townPtr->m_army.m_creatureTypes[creatureSlot];
            stackIcons[wIndex] = new iconWidget(
                basePos + stride * wIndex + fiveShift,
                curY,
                ARMY_QUICK_ICON_SIZE,
                ARMY_QUICK_ICON_SIZE,
                "mons32.icn",
                monster,
                ICON_DRAW_NORMAL,
                WIDGET_ID_NONE,
                ICON_WIDGET_DRAW,
                1
            );
            if (!stackIcons[wIndex])
                MemError();
            troopNames[wIndex] = static_cast<char*>(malloc(ARMY_QUICK_SIZE_LABEL_CAPACITY));
            if (scouting == TOWN_QUICK_INFORMATION_EXACT)
                sprintf(troopNames[wIndex], "%d", townPtr->m_army.m_creatureCounts[creatureSlot]);
            else if (scouting == TOWN_QUICK_INFORMATION_ESTIMATES)
                strcpy(
                    troopNames[wIndex],
                    GetArmySizeName(
                        townPtr->m_army.m_creatureCounts[creatureSlot],
                        ARMY_SIZE_NAME_TITLE
                    )
                );
            else
                strcpy(troopNames[wIndex], "???");
            sizeWidgets[wIndex] = new textWidget(
                basePos + stride * wIndex + fiveShift - ARMY_QUICK_TEXT_X_ADJUSTMENT,
                curY + ARMY_QUICK_ICON_BASELINE,
                ARMY_QUICK_TEXT_WIDTH,
                ARMY_QUICK_LABEL_HEIGHT,
                troopNames[wIndex],
                "smalfont.fnt",
                1,
                WIDGET_ID_NONE,
                WIDGET_KIND_TEXT
            );
            if (!sizeWidgets[wIndex])
                MemError();
            window->AddWidget(stackIcons[wIndex], WINDOW_Z_ORDER_APPEND);
            window->AddWidget(sizeWidgets[wIndex], WINDOW_Z_ORDER_APPEND);
            wIndex++;
            creatureSlot++;
        }
        if (bottomRowCount) {
            stride = TOWN_QUICK_ARMY_AREA_WIDTH / bottomRowCount;
            basePos = (stride - ARMY_QUICK_ICON_SIZE) / 2 + ARMY_QUICK_AREA_LEFT;
            curY += ARMY_QUICK_SECOND_ROW_SHIFT;
            for (armyIndex = topRow; armyIndex < topRow + bottomRowCount; armyIndex++) {
                while (townPtr->m_army.m_creatureTypes[creatureSlot] == CREATURE_NONE)
                    creatureSlot++;
                monster = townPtr->m_army.m_creatureTypes[creatureSlot];
                stackIcons[wIndex] = new iconWidget(
                    basePos + stride * (wIndex - topRow),
                    curY,
                    ARMY_QUICK_ICON_SIZE,
                    ARMY_QUICK_ICON_SIZE,
                    "mons32.icn",
                    monster,
                    ICON_DRAW_NORMAL,
                    WIDGET_ID_NONE,
                    ICON_WIDGET_DRAW,
                    1
                );
                if (!stackIcons[wIndex])
                    MemError();
                troopNames[wIndex] = static_cast<char*>(malloc(ARMY_QUICK_SIZE_LABEL_CAPACITY));
                if (scouting == TOWN_QUICK_INFORMATION_EXACT)
                    sprintf(
                        troopNames[wIndex],
                        "%d",
                        townPtr->m_army.m_creatureCounts[creatureSlot]
                    );
                else if (scouting == TOWN_QUICK_INFORMATION_ESTIMATES)
                    strcpy(
                        troopNames[wIndex],
                        GetArmySizeName(
                            townPtr->m_army.m_creatureCounts[creatureSlot],
                            ARMY_SIZE_NAME_TITLE
                        )
                    );
                else
                    strcpy(troopNames[wIndex], "???");
                sizeWidgets[wIndex] = new textWidget(
                    basePos + stride * (wIndex - topRow) - ARMY_QUICK_TEXT_X_ADJUSTMENT,
                    curY + ARMY_QUICK_ICON_BASELINE,
                    ARMY_QUICK_TEXT_WIDTH,
                    ARMY_QUICK_LABEL_HEIGHT,
                    troopNames[wIndex],
                    "smalfont.fnt",
                    1,
                    WIDGET_ID_NONE,
                    WIDGET_KIND_TEXT
                );
                if (!sizeWidgets[wIndex])
                    MemError();
                window->AddWidget(stackIcons[wIndex], WINDOW_Z_ORDER_APPEND);
                window->AddWidget(sizeWidgets[wIndex], WINDOW_Z_ORDER_APPEND);
                wIndex++;
                creatureSlot++;
            }
        }
    }

    GrabScreen();
    gWindowManager->AddWindow(window, WINDOW_Z_ORDER_APPEND, 1);
    oldX = m_mapOriginX;
    oldY = m_mapOriginY;
    m_mapOriginX = townPtr->m_x - ADVMGR_VIEW_CENTER;
    m_mapOriginY = townPtr->m_y - ADVMGR_VIEW_CENTER;
    UpdateRadar(true, false);
    gMouseManager->HideSystemCursor();
    QuickViewWait();
    gWindowManager->RemoveWindow(window);
    delete window;
    gMouseManager->ShowSystemCursor();
    m_mapOriginX = oldX;
    m_mapOriginY = oldY;
    UpdateRadar(true, false);
    CompleteDraw(false);
    UpdateScreen(false, false);
    if (message.type == MESSAGE_LEFT_BUTTON_DOWN && townPtr->m_owner == gCurPlayer)
        SetTownContext(townPtr->m_id);
}

void advManager::RedrawAdvScreen(b32 update) {
    if (!gShowIt)
        return;
    gResourceManager->GetBackdrop("bord.bmp", gWindowManager->m_screen);
    SaveAdventureBorder();
    UpdateHeroLocators(false, 0);
    UpdateTownLocators(false, 0);
    UpdBottomView(true, false, false);
    m_adventureWindow->DrawWindow(0);
    if (update)
        gWindowManager->UpdateScreenRegion(0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT);
    UpdateRadar(update, false);
    CompleteDraw(m_mapOriginX, m_mapOriginY, false);
    if (update)
        UpdateScreen(false, false);
}

void advManager::DeactivateCurrTown(void) {
    gCurPlayerData->m_currentTown = GAME_TOWN_NONE;
}

void advManager::DeactivateCurrHero(void) {
    DemobilizeCurrHero();
    gCurPlayerData->m_currentHero = HERO_ID_NONE;
}

void advManager::MobilizeCurrHero(b32 update) {
    if (gCurPlayerData->m_currentHero == HERO_ID_NONE)
        return;
    if (m_heroContextLocked)
        return;
    SetHeroContext(gCurPlayerData->m_currentHero, update);
}

void advManager::DemobilizeCurrHero(void) {
    if (gCurPlayerData->m_currentHero == HERO_ID_NONE)
        return;
    if (!m_heroContextLocked)
        return;

    m_heroContextLocked = false;
    hero* heroPointer = gGame->GetHero(gCurPlayerData->m_currentHero);
    StopCursor(true);
    heroPointer->m_x = m_mapOriginX + m_cursorMapX;
    heroPointer->m_y = m_mapOriginY + m_cursorMapY;
    mapCell* cell = GetCell(heroPointer->m_x, heroPointer->m_y);
    heroPointer->m_locationType = cell->m_triggerType;
    heroPointer->m_occupiedTown = cell->m_objectMetadata;
    heroPointer->m_direction = m_cursorDirection;
    if (m_cursorType == ADVMGR_HERO_ICON_BOAT)
        heroPointer->m_eventFlags |= HERO_EVENT_EMBARKED;
    cell->m_triggerType = MAP_EVENT_TRIGGER(MAP_OBJECT_HERO);
    cell->m_objectMetadata = heroPointer->m_id;
    cell->m_flags &= ~MAP_CELL_HERO_CURSOR;
    m_cursorActive = false;
    CompleteDraw(m_mapOriginX, m_mapOriginY, false);
    UpdateScreen(false, false);
}

void advManager::SetTownContext(i8 townId) {
    i16 index;
    i8 townNo;
    i8 wasVisible;
    town* townPointer;

    DeactivateCurrHero();
    wasVisible = gMouseManager->IsVis();
    gMouseManager->ReallyHidePointer();
    gCurPlayerData->m_currentTown = townId;
    townPointer = gGame->GetTown(gCurPlayerData->m_currentTown);
    m_mapOriginX = townPointer->m_x - ADVMGR_VIEW_CENTER;
    m_mapOriginY = townPointer->m_y - ADVMGR_VIEW_CENTER;
    townNo = 0;
    for (index = 0; index < gCurPlayerData->m_townCount; index++) {
        if (gCurPlayerData->m_townIds[index] == townId)
            townNo = index;
    }
    if (townNo < gCurPlayerData->m_townLocatorPage)
        gCurPlayerData->m_townLocatorPage = townNo;
    else if (townNo > gCurPlayerData->m_townLocatorPage + (LOCATOR_VISIBLE_COUNT - 1))
        gCurPlayerData->m_townLocatorPage = townNo - (LOCATOR_VISIBLE_COUNT - 1);
    UpdateHeroLocators(true, 1);
    UpdateTownLocators(true, 1);
    HideRoute(false, false, true);
    UpdBottomView(true, true, true);
    UpdateRadar(true, false);
    CompleteDraw(m_mapOriginX, m_mapOriginY, false);
    UpdateScreen(false, false);
    SetEnvironmentOrigin(m_mapOriginX + ADVMGR_VIEW_CENTER, m_mapOriginY + ADVMGR_VIEW_CENTER, 1);
    townNo = (CELL_TERRAIN(GetCell(townPointer->m_x, townPointer->m_y)));
    if (townNo != m_currentTerrain) {
        m_currentTerrain = townNo;
        PlayMusic(TERRAIN_MUSIC_TRACK(m_currentTerrain));
    }
    if (wasVisible)
        gMouseManager->ReallyShowPointer();
    gInputManager->m_forceMouseMove = true;
    m_hoverCellX = 0;
}

void advManager::SetHeroContext(i8 heroId, b8 update) {
    i8 wasVisible;
    i8 curHeroSlot;
    i16 index;
    mapCell* currCell;
    hero* heroPtr;

    if (heroId == HERO_ID_NONE)
        return;
    wasVisible = gMouseManager->IsVis();
    gMouseManager->ReallyHidePointer();
    DeactivateCurrTown();
    HideRoute(false, false, true);
    DeactivateCurrHero();
    m_heroContextLocked = true;
    gCurPlayerData->m_currentHero = heroId;
    heroPtr = gGame->GetHero(gCurPlayerData->m_currentHero);
    m_mapOriginX = heroPtr->m_x - ADVMGR_VIEW_CENTER;
    m_mapOriginY = heroPtr->m_y - ADVMGR_VIEW_CENTER;
    m_cursorMapX = m_cursorMapY = ADVMGR_VIEW_CENTER;
    m_previousCursorMapX = m_previousCursorMapY = CURSOR_CELL_NONE;
    m_cursorType = heroPtr->IsEmbarked()
                       ? static_cast<i8>(ADVMGR_HERO_ICON_BOAT)
                       : heroPtr->m_heroClass;
    m_cursorDirection = heroPtr->m_direction;
    m_cursorFrame = GetCursorBaseFrame(m_cursorDirection);
    currCell = GetCell(heroPtr->m_x, heroPtr->m_y);
    currCell->m_flags |= MAP_CELL_HERO_CURSOR;
    gGame->RestoreCell(
        heroPtr->m_x,
        heroPtr->m_y,
        heroPtr->m_locationType,
        heroPtr->m_occupiedTown,
        NULL,
        4
    );
    curHeroSlot = 0;
    for (index = 0; index < gCurPlayerData->m_heroCount; index++) {
        if (gCurPlayerData->m_heroIds[index] == heroId)
            curHeroSlot = index;
    }
    if (curHeroSlot < gCurPlayerData->m_heroLocatorPage)
        gCurPlayerData->m_heroLocatorPage = curHeroSlot;
    else if (curHeroSlot > gCurPlayerData->m_heroLocatorPage + (LOCATOR_VISIBLE_COUNT - 1))
        gCurPlayerData->m_heroLocatorPage = curHeroSlot - (LOCATOR_VISIBLE_COUNT - 1);
    UpdateHeroLocators(true, 1);
    UpdateTownLocators(true, 1);
    if (!update && (m_active == 1 || gThisNetHumanPlayer[gCurPlayer])) {
        Reseed(0, 0);
        SeedTo(heroPtr->m_destinationX, heroPtr->m_destinationY);
        ShowRoute(false, 0, !update);
    }
    UpdBottomView(true, true, true);
    m_cursorActive = true;
    UpdateRadar(true, false);
    CompleteDraw(m_mapOriginX, m_mapOriginY, false);
    UpdateScreen(false, false);
    SetEnvironmentOrigin(m_mapOriginX + ADVMGR_VIEW_CENTER, m_mapOriginY + ADVMGR_VIEW_CENTER, 1);
    curHeroSlot = (CELL_TERRAIN(currCell));
    if (curHeroSlot != m_currentTerrain) {
        m_currentTerrain = curHeroSlot;
        PlayMusic(TERRAIN_MUSIC_TRACK(m_currentTerrain));
    }
    if (!gHeroMoving) {
        if (wasVisible)
            gMouseManager->ReallyShowPointer();
        gInputManager->m_forceMouseMove = true;
        m_hoverCellX = 0;
    }
}

void advManager::DoHeroKnob(void) {
    i8 prevPage;
    i16 count;
    i16 pageIndex;
    double scale;
    i16 x;
    i16 y;
    i16 offset;
    tag_message message;

    gMouseManager->SetCursorShape(4);
    prevPage = gCurPlayerData->m_heroLocatorPage;
    count = gCurPlayerData->m_heroCount;
    scale = 73.0 / (count - LOCATOR_PAGE_DENOMINATOR_OFFSET);
    gMouseManager->MouseCoords(x, y);
    offset = y - m_scrollLeftButton->m_y;
    gInputManager->Flush();
    message = gInputManager->GetEvent();
    while (!IS_BUTTON_RELEASE_MESSAGE(message.type)) {
        if (message.type == MESSAGE_MOUSE_MOVE) {
            if (message.y < offset + LOCATOR_SCROLL_BASE_Y)
                message.y = offset + LOCATOR_SCROLL_BASE_Y;
            if (message.y > offset + LOCATOR_SCROLL_BASE_Y + LOCATOR_HERO_SCROLL_SPAN)
                message.y = offset + LOCATOR_SCROLL_BASE_Y + LOCATOR_HERO_SCROLL_SPAN;
            gMouseManager->Main(message);
            m_scrollLeftButton->m_y = message.y - offset;
            m_adventureWindow->DrawWindow();
            if (count > LOCATOR_VISIBLE_COUNT) {
                pageIndex = (m_scrollLeftButton->m_y - LOCATOR_SCROLL_BASE_Y) / scale;
                if (pageIndex != prevPage) {
                    gCurPlayerData->m_heroLocatorPage = pageIndex;
                    if (pageIndex > count - (LOCATOR_VISIBLE_COUNT - 1))
                        pageIndex = count - (LOCATOR_VISIBLE_COUNT - 1);
                    UpdateHeroLocators(false, 1);
                    m_scrollLeftButton->m_y = message.y - offset;
                    m_adventureWindow->DrawWindow();
                    prevPage = pageIndex;
                }
            }
        }
        Process1WindowsMessage();
        message = gInputManager->GetEvent();
    }
    gMouseManager->SetCursorShape(6);
    m_scrollLeftButton->m_flags &= ~WIDGET_FLAG_SELECTED;
    UpdateHeroLocators(true, 1);
}

void advManager::DoTownKnob(void) {
    i8 prevPage;
    i16 count;
    i16 pageIndex;
    double scale;
    i16 x;
    i16 y;
    i16 offset;
    tag_message message;

    gMouseManager->SetCursorShape(4);
    prevPage = gCurPlayerData->m_townLocatorPage;
    count = gCurPlayerData->m_townCount;
    scale = 74.0 / (count - LOCATOR_PAGE_DENOMINATOR_OFFSET);
    gMouseManager->MouseCoords(x, y);
    offset = y - m_scrollRightButton->m_y;
    gInputManager->Flush();
    message = gInputManager->GetEvent();
    while (!IS_BUTTON_RELEASE_MESSAGE(message.type)) {
        if (message.type == MESSAGE_MOUSE_MOVE) {
            if (message.y < offset + LOCATOR_SCROLL_BASE_Y)
                message.y = offset + LOCATOR_SCROLL_BASE_Y;
            if (message.y > offset + LOCATOR_SCROLL_BASE_Y + LOCATOR_TOWN_SCROLL_SPAN)
                message.y = offset + LOCATOR_SCROLL_BASE_Y + LOCATOR_TOWN_SCROLL_SPAN;
            gMouseManager->Main(message);
            m_scrollRightButton->m_y = message.y - offset;
            m_adventureWindow->DrawWindow();
            if (count > LOCATOR_VISIBLE_COUNT) {
                pageIndex = (m_scrollRightButton->m_y - LOCATOR_SCROLL_BASE_Y) / scale;
                if (pageIndex != prevPage) {
                    gCurPlayerData->m_townLocatorPage = pageIndex;
                    if (pageIndex > count - (LOCATOR_VISIBLE_COUNT - 1))
                        pageIndex = count - (LOCATOR_VISIBLE_COUNT - 1);
                    UpdateTownLocators(false, 1);
                    m_scrollRightButton->m_y = message.y - offset;
                    m_adventureWindow->DrawWindow();
                    prevPage = pageIndex;
                }
            }
        }
        Process1WindowsMessage();
        message = gInputManager->GetEvent();
    }
    gMouseManager->SetCursorShape(6);
    m_scrollRightButton->m_flags &= ~WIDGET_FLAG_SELECTED;
    UpdateTownLocators(true, 1);
}

void advManager::ViewPuzzle(void) {
    i32 puzzleX;
    i32 puzzleY;
    i8 visibleCount;
    icon* puzzleIcn;
    heroWindow* pWin;
    i16 j;

    visibleCount = 0;
    PlayMusic(MUSIC_TRACK_PUZZLE);
    gMouseManager->SetPointer("advmice.mse", ADVENTURE_POINTER_DEFAULT);
    puzzleIcn = gResourceManager->GetIcon("puzzle.icn");
    for (j = 0; j < PUZZLE_PIECE_COUNT; j++)
        puzzleIcn->DrawToBuffer(0, 0, j, ICON_DRAW_NORMAL, ICON_DRAW_OFFSET_FULL);
    gWindowManager->UpdateScreenRegion(
        BORDER_EDGE_SIZE,
        BORDER_EDGE_SIZE,
        ADVENTURE_VIEWPORT_INNER_SIZE,
        ADVENTURE_VIEWPORT_INNER_SIZE
    );
    gWindowManager->SaveFizzleSource(
        BORDER_EDGE_SIZE,
        BORDER_EDGE_SIZE,
        ADVENTURE_VIEWPORT_INNER_SIZE,
        ADVENTURE_VIEWPORT_INNER_SIZE
    );
    pWin = new heroWindow(RADAR_LEFT, RADAR_TOP, "viewpuzl.bin");
    if (!pWin)
        MemError();
    gWindowManager->AddWindow(pWin, WINDOW_Z_ORDER_APPEND, 1);

    puzzleX = gGame->m_ultimateArtifactX - ADVMGR_VIEW_CENTER;
    puzzleY = gGame->m_ultimateArtifactY - ADVMGR_VIEW_CENTER;
    i32 xOff = 0;
    i32 yOff = 0;
    xOff = (gGame->m_ultimateArtifactX + gGame->m_ultimateArtifactY) % PUZZLE_ALIGNMENT_DIVISOR - 1;
    yOff = (gGame->m_ultimateArtifactX * PUZZLE_Y_ADJUST_X_FACTOR
            + gGame->m_ultimateArtifactY * PUZZLE_Y_ADJUST_Y_FACTOR)
               % PUZZLE_ALIGNMENT_DIVISOR
           - 1;
    if ((gGame->m_ultimateArtifactX + gGame->m_ultimateArtifactY) % PUZZLE_ALIGNMENT_DIVISOR == 1) {
        if (xOff > 0)
            xOff++;
        else if (xOff < 0)
            xOff--;
    } else if ((gGame->m_ultimateArtifactX + gGame->m_ultimateArtifactY) % PUZZLE_PARITY_DIVISOR
               == 1) {
        if (yOff > 0)
            yOff++;
        else if (yOff < 0)
            yOff--;
    }
    puzzleX += xOff;
    puzzleY += yOff;
    PuzzleDraw(puzzleX, puzzleY, gGame->m_ultimateArtifactX, gGame->m_ultimateArtifactY);

    for (j = 0; j < PUZZLE_PIECE_COUNT; j++) {
        if (!BitTest(gCurPlayerData->m_puzzlePiecesRemoved, j)) {
            puzzleIcn->DrawToBuffer(0, 0, j, ICON_DRAW_NORMAL, ICON_DRAW_OFFSET_FULL);
            visibleCount++;
        }
    }
    if (visibleCount != PUZZLE_PIECE_COUNT) {
        gMouseManager->ReallyHidePointer();
        gWindowManager->FizzleForward(
            BORDER_EDGE_SIZE,
            BORDER_EDGE_SIZE,
            ADVENTURE_VIEWPORT_INNER_SIZE,
            ADVENTURE_VIEWPORT_INNER_SIZE,
            PUZZLE_FIZZLE_TIME
        );
        gMouseManager->ReallyShowPointer();
    } else {
        gWindowManager->ReleaseFizzleSource();
    }

    gWindowManager->DoDialog(pWin, EventWindowHandler, false);
    delete pWin;
    CompleteDraw(m_mapOriginX, m_mapOriginY, false);
    UpdateScreen(false, false);
    UpdateRadar(true, false);
    PlayMusic(TERRAIN_MUSIC_TRACK(m_currentTerrain));
}

void advManager::PuzzleDraw(i32 left, i32 top, i32 markX, i32 markY) {
    i32 y;
    mapCell* cell;
    i32 x;
    u8 tileset;
    i16 screenX;
    i16 rowPixelY;

    for (y = 0; y < ADVMGR_VIEW_CELL_COUNT; y++) {
        for (x = 0; x < ADVMGR_VIEW_CELL_COUNT; x++) {
            DrawCell(left + x, top + y, x, y, ADVMGR_DRAW_GROUND, true, false);
            // Off the map the stone border has nothing on it.
            if (!MAP_CELL_IN_BOUNDS(left + x, top + y))
                continue;
            screenX = x * CELL_PIXELS;
            rowPixelY = y * CELL_PIXELS;
            cell = GetCell(left + x, top + y);
            if (!(cell->m_flags & MAP_CELL_OBJECT_SHADOW_ONLY)
                && cell->m_objectIndex != MAP_CELL_NO_FRAME) {
                tileset = (cell->m_objectTileset & MAP_CELL_TILESET_MASK);
                switch (tileset) {
                    case TILESET_OBJ32_04:
                    case TILESET_MTN32:
                    case TILESET_TREE32:
                        IconToBitmap(
                            m_objectIcons[tileset],
                            gWindowManager->m_screen,
                            screenX,
                            rowPixelY,
                            cell->m_objectIndex,
                            ICON_DRAW_OFFSET_FULL
                        );
                        break;
                    default:
                        break;
                }
            }
            if (cell->m_overlayIndex != MAP_CELL_NO_FRAME) {
                tileset =
                    (cell->m_overlayTileset & MAP_CELL_TILESET_MASK);
                switch (tileset) {
                    case TILESET_OBJ32_04:
                    case TILESET_MTN32:
                    case TILESET_TREE32:
                        IconToBitmap(
                            m_objectIcons[tileset],
                            gWindowManager->m_screen,
                            screenX,
                            rowPixelY,
                            cell->m_overlayIndex,
                            ICON_DRAW_OFFSET_FULL
                        );
                        break;
                    default:
                        break;
                }
            }
            if (left + x == markX && top + y == markY)
                IconToBitmap(
                    m_objectIcons[TILESET_ROUTE],
                    gWindowManager->m_screen,
                    screenX,
                    rowPixelY + ROUTE_DRAW_Y_OFFSET,
                    ROUTE_CELL_DESTINATION - 1,
                    ICON_DRAW_OFFSET_FULL
                );
        }
    }
    DrawAdventureBorder();
}

void advManager::CastSpell(i8 spell) {
    hero* caster;
    i32 guardianCount;

    if (gCurPlayerData->CurrentHero() != HERO_ID_NONE)
        caster = gGame->GetHero(gCurPlayerData->m_currentHero);
    else
        caster = NULL;

    switch (spell) {
        case SPELL_VIEW_MINES:
        case SPELL_VIEW_RESOURCES:
        case SPELL_VIEW_ARTIFACTS:
        case SPELL_VIEW_TOWNS:
        case SPELL_VIEW_HEROES:
        case SPELL_VIEW_ALL:
            ViewWorld(spell, true, spell == SPELL_VIEW_ALL);
            break;
        case SPELL_IDENTIFY_HERO:
            m_identifyHeroActive = true;
            NormalDialog(
                localization::Tr("adventure.spell.identify_hero"),
                NORMAL_DIALOG_TYPE_OK,
                0x61,
                0x91
            );
            break;
        case SPELL_SUMMON_BOAT:
            SummonBoat();
            break;
        case SPELL_DIMENSION_DOOR:
        case SPELL_TOWN_GATE:
            if (caster->m_remainingMobility == 0) {
                NormalDialog(localization::Tr("adventure.spell.too_tired"), NORMAL_DIALOG_TYPE_OK);
                return;
            }
            if (caster->m_remainingMobility < SPELL_TRAVEL_MOBILITY_COST)
                caster->m_remainingMobility = 0;
            else
                caster->m_remainingMobility -= SPELL_TRAVEL_MOBILITY_COST;
            UpdateHeroLocator(LOCATOR_SLOT_CURRENT_HERO, true, true);
            if (spell == SPELL_DIMENSION_DOOR)
                DimensionDoor();
            else
                TownGate();
            break;
        default:
            break;
    }

    if (spell != SPELL_DIMENSION_DOOR && spell != SPELL_TOWN_GATE)
        gGame->GetHero(gCurPlayerData->m_currentHero)->UseSpell(spell);
}

void advManager::ViewWorld(
    i8 spellType,
    b8 drawAllObjects,
    b8 drawAllTerrains
) {
    icon* flags;
    hero* currHero;
    i8 tileset;
    i8 flip;
    icon* lettersIcons;
    i16 index;
    heroWindow* win;
    u16 mask;
    i16 x;
    i16 owner;
    mapCell* cell;
    icon* mapTilesets[VIEW_WORLD_TILESET_COUNT];
    i16 i;
    i16 y;
    i16 screenX;
    icon* spheres;
    i16 rowPixelY;
    icon* ground;

    gMouseManager->SetPointer("advmice.mse", ADVENTURE_POINTER_DEFAULT);
    mask = (1 << TILESET_MTN32) | (1 << TILESET_TREE32);
    if (spellType == SPELL_VIEW_TOWNS || spellType == SPELL_VIEW_ALL)
        mask |= (1 << TILESET_TOWN32);
    ground = gResourceManager->GetIcon("ground6.icn");
    flags = gResourceManager->GetIcon("flag6.icn");
    spheres = gResourceManager->GetIcon("spheres.icn");
    lettersIcons = gResourceManager->GetIcon("letters.icn");
    currHero = NULL;
    for (i = 0; i < VIEW_WORLD_TILESET_COUNT; i++)
        mapTilesets[i] = NULL;
    mapTilesets[TILESET_TREE32] = gResourceManager->GetIcon("tree6.icn");
    mapTilesets[TILESET_MTN32] = gResourceManager->GetIcon("mtn6.icn");
    mapTilesets[TILESET_TOWN32] = gResourceManager->GetIcon("town6.icn");
    if (gCurPlayerData->CurrentHero() != HERO_ID_NONE)
        currHero = gGame->GetHero(gCurPlayerData->m_currentHero);
    FillBitmapArea(
        gWindowManager->m_screen,
        BORDER_EDGE_SIZE,
        BORDER_EDGE_SIZE,
        ADVENTURE_VIEWPORT_INNER_SIZE,
        ADVENTURE_VIEWPORT_INNER_SIZE,
        0
    );

    for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
        for (x = MAP_CELL_GRID_SIZE - 1; x >= 0; x--) {
            cell = GetCell(x, y);
            if ((gGame->m_mapExtra[x][y] & gCurPlayerBit) || drawAllTerrains
                || (spellType == SPELL_VIEW_TOWNS
                    && MAP_TRIGGER_OBJECT(cell->m_triggerType) == MAP_OBJECT_TOWN)) {
                flip = ICON_DRAW_NORMAL;
                screenX = x * VIEW_WORLD_CELL_PIXELS + VIEW_WORLD_ORIGIN;
                rowPixelY = y * VIEW_WORLD_CELL_PIXELS + VIEW_WORLD_ORIGIN;
                index = cell->m_tileIndex / (1 << VIEW_WORLD_GROUND_TILE_SHIFT);
                if (cell->m_flags & MAP_CELL_GROUND_FLIP_HORIZONTAL)
                    flip = ICON_DRAW_FLIPPED;
                if (cell->m_flags & MAP_CELL_GROUND_FLIP_VERTICAL)
                    index += VIEW_WORLD_GROUND_FLIPPED_FRAMES;
                ground->DrawToBuffer(
                    screenX + (flip == ICON_DRAW_FLIPPED ? VIEW_WORLD_CELL_PIXELS - 1 : 0),
                    rowPixelY,
                    index,
                    flip,
                    ICON_DRAW_OFFSET_FULL
                );
                if (cell->m_objectIndex != MAP_CELL_NO_FRAME) {
                    tileset = (cell->m_objectTileset & MAP_CELL_TILESET_MASK);
                    // The small trees have fewer frames than the map's
                    // trees (98 against 101); the original drew the last
                    // three from a frame entry past the table.
                    if (((1 << tileset) & mask)
                        && cell->m_objectIndex < mapTilesets[tileset]->m_frameCount)
                        mapTilesets[tileset]->DrawToBuffer(
                            screenX,
                            rowPixelY,
                            cell->m_objectIndex,
                            ICON_DRAW_NORMAL,
                            ICON_DRAW_OFFSET_FULL
                        );
                }
            }
        }
        for (x = MAP_CELL_GRID_SIZE - 1; x >= 0; x--) {
            cell = GetCell(x, y);
            screenX = x * VIEW_WORLD_CELL_PIXELS + VIEW_WORLD_ORIGIN;
            rowPixelY = y * VIEW_WORLD_CELL_PIXELS + VIEW_WORLD_ORIGIN;
            if ((drawAllObjects || (gGame->m_mapExtra[x][y] & gCurPlayerBit))
                && (cell->m_triggerType & MAP_TRIGGER_EVENT)) {
                switch (spellType) {
                    case SPELL_VIEW_ALL:
                        if (cell->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_ARTIFACT))
                            flags->DrawToBuffer(
                                screenX,
                                rowPixelY,
                                VIEW_WORLD_FLAG_ARTIFACT,
                                ICON_DRAW_NORMAL,
                                ICON_DRAW_OFFSET_FULL
                            );
                        if (cell->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_TOWN)) {
                            owner = gGame->m_townOwners[cell->m_objectMetadata];
                            if (owner >= 0) {
                                index =
                                    (gGame->m_players[owner].m_color);
                                flags->DrawToBuffer(
                                    screenX - VIEW_WORLD_TOWN_FLAG_LEFT,
                                    rowPixelY,
                                    index,
                                    ICON_DRAW_FLIPPED,
                                    ICON_DRAW_OFFSET_FULL
                                );
                                flags->DrawToBuffer(
                                    screenX + VIEW_WORLD_TOWN_FLAG_RIGHT,
                                    rowPixelY,
                                    index,
                                    ICON_DRAW_NORMAL,
                                    ICON_DRAW_OFFSET_FULL
                                );
                            }
                        } else if (cell->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_HERO)
                                   && gGame->m_heroRecs[cell->m_objectMetadata].m_locationType
                                          == MAP_EVENT_TRIGGER(MAP_OBJECT_TOWN)) {
                            owner = gGame->m_townOwners[gGame->m_heroRecs[cell->m_objectMetadata]
                                                            .m_occupiedTown];
                            if (owner >= 0) {
                                index =
                                    (gGame->m_players[owner].m_color);
                                flags->DrawToBuffer(
                                    screenX - VIEW_WORLD_TOWN_FLAG_LEFT,
                                    rowPixelY,
                                    index,
                                    ICON_DRAW_FLIPPED,
                                    ICON_DRAW_OFFSET_FULL
                                );
                                flags->DrawToBuffer(
                                    screenX + VIEW_WORLD_TOWN_FLAG_RIGHT,
                                    rowPixelY,
                                    index,
                                    ICON_DRAW_NORMAL,
                                    ICON_DRAW_OFFSET_FULL
                                );
                            }
                        }
                        switch (MAP_TRIGGER_OBJECT(cell->m_triggerType)) {
                            case MAP_OBJECT_ALCHEMIST_LAB:
                            case MAP_OBJECT_MINE:
                            case MAP_OBJECT_SAWMILL:
                                owner = gGame->m_mineOwners[cell->m_objectMetadata];
                                index = owner < 0
                                            ? PLAYER_COLOR_NEUTRAL
                                            : (gGame->m_players[owner].m_color);
                                spheres->DrawToBuffer(
                                    screenX,
                                    rowPixelY,
                                    index,
                                    ICON_DRAW_NORMAL,
                                    ICON_DRAW_OFFSET_FULL
                                );
                                lettersIcons->DrawToBuffer(
                                    screenX,
                                    rowPixelY,
                                    (gGame->m_mines[cell->m_objectMetadata].type),
                                    ICON_DRAW_NORMAL,
                                    ICON_DRAW_OFFSET_FULL
                                );
                                break;
                            case MAP_OBJECT_HERO:
                                switch (MAP_TRIGGER_OBJECT(
                                    gGame->m_heroRecs[cell->m_objectMetadata].m_locationType
                                )) {
                                    case MAP_OBJECT_ALCHEMIST_LAB:
                                    case MAP_OBJECT_MINE:
                                    case MAP_OBJECT_SAWMILL:
                                        owner = gGame->m_mineOwners
                                                    [gGame->m_heroRecs[cell->m_objectMetadata]
                                                         .m_occupiedTown];
                                        index =
                                            owner < 0
                                                ? PLAYER_COLOR_NEUTRAL
                                                : (gGame->m_players[owner].m_color);
                                        spheres->DrawToBuffer(
                                            screenX,
                                            rowPixelY,
                                            index,
                                            ICON_DRAW_NORMAL,
                                            ICON_DRAW_OFFSET_FULL
                                        );
                                        // A hero's cell holds the hero, not the mine
                                        // (Tournament Edition TE-FIX-8); the original
                                        // drew the letter of mine record number
                                        // hero id, up to frame 23 of 7.
                                        lettersIcons->DrawToBuffer(
                                            screenX,
                                            rowPixelY,
                                            (gGame
                                                ->m_mines[gGame->m_heroRecs[cell->m_objectMetadata]
                                                              .m_occupiedTown]
                                                .type),
                                            ICON_DRAW_NORMAL,
                                            ICON_DRAW_OFFSET_FULL
                                        );
                                        break;
                                    default:
                                        break;
                                }
                        }
                        if (cell->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_HERO)) {
                            owner = gGame->m_availableHeroes[cell->m_objectMetadata];
                            if (owner >= 0) {
                                index =
                                    (gGame->m_players[owner].m_color);
                                flags->DrawToBuffer(
                                    screenX,
                                    rowPixelY,
                                    index,
                                    ICON_DRAW_NORMAL,
                                    ICON_DRAW_OFFSET_FULL
                                );
                            }
                        }
                        break;
                    case SPELL_VIEW_MINES:
                        switch (MAP_TRIGGER_OBJECT(cell->m_triggerType)) {
                            case MAP_OBJECT_ALCHEMIST_LAB:
                            case MAP_OBJECT_MINE:
                            case MAP_OBJECT_SAWMILL:
                                owner = gGame->m_mineOwners[cell->m_objectMetadata];
                                index = owner < 0
                                            ? PLAYER_COLOR_NEUTRAL
                                            : (gGame->m_players[owner].m_color);
                                spheres->DrawToBuffer(
                                    screenX,
                                    rowPixelY,
                                    index,
                                    ICON_DRAW_NORMAL,
                                    ICON_DRAW_OFFSET_FULL
                                );
                                lettersIcons->DrawToBuffer(
                                    screenX,
                                    rowPixelY,
                                    (gGame->m_mines[cell->m_objectMetadata].type),
                                    ICON_DRAW_NORMAL,
                                    ICON_DRAW_OFFSET_FULL
                                );
                                break;
                            case MAP_OBJECT_HERO:
                                switch (MAP_TRIGGER_OBJECT(
                                    gGame->m_heroRecs[cell->m_objectMetadata].m_locationType
                                )) {
                                    case MAP_OBJECT_ALCHEMIST_LAB:
                                    case MAP_OBJECT_MINE:
                                    case MAP_OBJECT_SAWMILL:
                                        owner = gGame->m_mineOwners
                                                    [gGame->m_heroRecs[cell->m_objectMetadata]
                                                         .m_occupiedTown];
                                        index =
                                            owner < 0
                                                ? PLAYER_COLOR_NEUTRAL
                                                : (gGame->m_players[owner].m_color);
                                        spheres->DrawToBuffer(
                                            screenX,
                                            rowPixelY,
                                            index,
                                            ICON_DRAW_NORMAL,
                                            ICON_DRAW_OFFSET_FULL
                                        );
                                        // A hero's cell holds the hero, not the mine
                                        // (Tournament Edition TE-FIX-8); the original
                                        // drew the letter of mine record number
                                        // hero id, up to frame 23 of 7.
                                        lettersIcons->DrawToBuffer(
                                            screenX,
                                            rowPixelY,
                                            (gGame
                                                ->m_mines[gGame->m_heroRecs[cell->m_objectMetadata]
                                                              .m_occupiedTown]
                                                .type),
                                            ICON_DRAW_NORMAL,
                                            ICON_DRAW_OFFSET_FULL
                                        );
                                        break;
                                    default:
                                        break;
                                }
                                break;
                            default:
                                break;
                        }
                        break;
                    case SPELL_VIEW_RESOURCES:
                        if (cell->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_RESOURCE)) {
                            spheres->DrawToBuffer(
                                screenX - VIEW_WORLD_RESOURCE_X_SHIFT,
                                rowPixelY,
                                PLAYER_COLOR_NEUTRAL,
                                ICON_DRAW_NORMAL,
                                ICON_DRAW_OFFSET_FULL
                            );
                            lettersIcons->DrawToBuffer(
                                screenX - VIEW_WORLD_RESOURCE_X_SHIFT,
                                rowPixelY,
                                cell->m_objectIndex - RESOURCE_PILE_OBJECT_BASE,
                                ICON_DRAW_NORMAL,
                                ICON_DRAW_OFFSET_FULL
                            );
                        }
                        break;
                    case SPELL_VIEW_ARTIFACTS:
                        if (cell->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_ARTIFACT))
                            flags->DrawToBuffer(
                                screenX,
                                rowPixelY,
                                VIEW_WORLD_FLAG_ARTIFACT,
                                ICON_DRAW_NORMAL,
                                ICON_DRAW_OFFSET_FULL
                            );
                        break;
                    case SPELL_VIEW_TOWNS:
                        if (cell->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_TOWN)) {
                            owner = gGame->m_townOwners[cell->m_objectMetadata];
                            if (owner >= 0) {
                                index =
                                    (gGame->m_players[owner].m_color);
                                flags->DrawToBuffer(
                                    screenX - VIEW_WORLD_TOWN_FLAG_LEFT,
                                    rowPixelY,
                                    index,
                                    ICON_DRAW_FLIPPED,
                                    ICON_DRAW_OFFSET_FULL
                                );
                                flags->DrawToBuffer(
                                    screenX + VIEW_WORLD_TOWN_FLAG_RIGHT,
                                    rowPixelY,
                                    index,
                                    ICON_DRAW_NORMAL,
                                    ICON_DRAW_OFFSET_FULL
                                );
                            }
                        } else if (cell->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_HERO)
                                   && gGame->m_heroRecs[cell->m_objectMetadata].m_locationType
                                          == MAP_EVENT_TRIGGER(MAP_OBJECT_TOWN)) {
                            owner = gGame->m_townOwners[gGame->m_heroRecs[cell->m_objectMetadata]
                                                            .m_occupiedTown];
                            if (owner >= 0) {
                                index =
                                    (gGame->m_players[owner].m_color);
                                flags->DrawToBuffer(
                                    screenX - VIEW_WORLD_TOWN_FLAG_LEFT,
                                    rowPixelY,
                                    index,
                                    ICON_DRAW_FLIPPED,
                                    ICON_DRAW_OFFSET_FULL
                                );
                                flags->DrawToBuffer(
                                    screenX + VIEW_WORLD_TOWN_FLAG_RIGHT,
                                    rowPixelY,
                                    index,
                                    ICON_DRAW_NORMAL,
                                    ICON_DRAW_OFFSET_FULL
                                );
                            }
                        }
                        break;
                    case SPELL_VIEW_HEROES:
                        if (cell->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_HERO)) {
                            owner = gGame->m_availableHeroes[cell->m_objectMetadata];
                            if (owner >= 0) {
                                index =
                                    (gGame->m_players[owner].m_color);
                                flags->DrawToBuffer(
                                    screenX,
                                    rowPixelY,
                                    index,
                                    ICON_DRAW_NORMAL,
                                    ICON_DRAW_OFFSET_FULL
                                );
                            }
                        }
                        break;
                    default:
                        break;
                }
            }
            if (currHero && currHero->m_x == x && currHero->m_y == y)
                flags->DrawToBuffer(
                    screenX,
                    rowPixelY,
                    VIEW_WORLD_FLAG_CURRENT_HERO,
                    ICON_DRAW_NORMAL,
                    ICON_DRAW_OFFSET_FULL
                );
        }
        for (x = MAP_CELL_GRID_SIZE - 1; x >= 0; x--) {
            cell = GetCell(x, y);
            if ((gGame->m_mapExtra[x][y] & gCurPlayerBit) || drawAllTerrains
                || (cell->m_triggerType == MAP_OBJECT_TRIGGER(MAP_OBJECT_TOWN)
                    && spellType == SPELL_VIEW_TOWNS)) {
                screenX = x * VIEW_WORLD_CELL_PIXELS + VIEW_WORLD_ORIGIN;
                rowPixelY = y * VIEW_WORLD_CELL_PIXELS + VIEW_WORLD_ORIGIN;
                if (cell->m_overlayIndex != MAP_CELL_NO_FRAME) {
                    tileset = (cell->m_overlayTileset & MAP_CELL_TILESET_MASK);
                    // The small trees have fewer frames than the map's
                    // trees (98 against 101); the original drew the last
                    // three from a frame entry past the table.
                    if (((1 << tileset) & mask)
                        && cell->m_objectIndex < mapTilesets[tileset]->m_frameCount)
                        mapTilesets[tileset]->DrawToBuffer(
                            screenX,
                            rowPixelY,
                            cell->m_overlayIndex,
                            ICON_DRAW_NORMAL,
                            ICON_DRAW_OFFSET_FULL
                        );
                }
            }
        }
    }

    gWindowManager->UpdateScreenRegion(
        BORDER_EDGE_SIZE,
        BORDER_EDGE_SIZE,
        ADVENTURE_VIEWPORT_INNER_SIZE,
        ADVENTURE_VIEWPORT_INNER_SIZE
    );
    sprintf(
        gText,
        "view-%02d.bin",
        spellType - SPELL_VIEW_MINES
    );
    win = new heroWindow(RADAR_LEFT, RADAR_TOP, gText);
    if (!win)
        MemError();
    gWindowManager->DoDialog(win, TrueFalseDialogHandler, false);
    delete win;
    UpdateRadar(true, false);
    for (i = 0; i < VIEW_WORLD_TILESET_COUNT; i++) {
        if (mapTilesets[i])
            gResourceManager->Dispose(mapTilesets[i]);
    }
    gResourceManager->Dispose(ground);
    gResourceManager->Dispose(flags);
    gResourceManager->Dispose(spheres);
    gResourceManager->Dispose(lettersIcons);
    RedrawAdvScreen(true);
}

void advManager::GrabScreen(void) {
    gMouseManager->ReallyHidePointer();
    GrabScreenBitmap(gWindowManager->m_screen, 0, 0);
    gMouseManager->ReallyShowPointer();
}

i16 advManager::ControlPanel(void) {
    tag_message message;
    b32 heroWasMobilized;
    i8 oldSpeedState;
    i32 gameCommand;
    i32 n;

    TrimLoopingSounds(ADVMGR_ACTIVE_SOUND_COUNT);
    gMouseManager->SetPointer("advmice.mse", ADVENTURE_POINTER_DEFAULT);
    gameCommand = MAIN_MENU_NO_COMMAND;
    oldSpeedState = gConfig.walkSpeed;
    gFreshSave = false;
    heroWasMobilized = m_heroContextLocked;
    gPrefsChanged = false;
    DemobilizeCurrHero();
    gAdventurePanel = new heroWindow(160, 10, "cpanel.bin");
    if (gAdventurePanel == NULL)
        MemError();
    SetWinText(gAdventurePanel, WINDOW_TEXT_CONTROL_PANEL);
    if (gRemoteOn) {
        message.type = MESSAGE_WIDGET;
        message.id = CONTROL_NEW_GAME;
        message.command = WIDGET_COMMAND_SET_FLAGS;
        message.value = WIDGET_FLAG_DIM_REQUEST;
        gAdventurePanel->BroadcastMessage(message);
        message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.value = WIDGET_FLAG_ENABLED;
        gAdventurePanel->BroadcastMessage(message);
        message.id = CONTROL_LOAD_GAME;
        message.command = WIDGET_COMMAND_SET_FLAGS;
        message.value = WIDGET_FLAG_DIM_REQUEST;
        gAdventurePanel->BroadcastMessage(message);
        message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.value = WIDGET_FLAG_ENABLED;
        gAdventurePanel->BroadcastMessage(message);
    }
    UpdateCPanel(true);
    gWindowManager->DoDialog(gAdventurePanel, CPanelHandler, false);
    delete gAdventurePanel;
    switch (gWindowManager->m_dialogResult) {
        case CONTROL_NEW_GAME:
        case CONTROL_LOAD_GAME:
        case CONTROL_QUIT:
            gameCommand = gWindowManager->m_dialogResult;
            break;
        case CONTROL_SCENARIO_INFO:
            if (gGame->m_campaignType > 0)
                gGame->ShowCampaignInfo(gGame->m_campaignScenario, true, 0);
            else
                gGame->ShowScenInfo();
            break;
        case CONTROL_SAVE_GAME:
            SaveGame();
            break;
    }
    if (gConfig.walkSpeed != oldSpeedState) {
        for (n = TERRAIN_WATER; n < TERRAIN_COUNT; n++) {
            if (m_cursorSamples[n]) {
                gResourceManager->Dispose(m_cursorSamples[n]);
                m_cursorSamples[n] = NULL;
            }
        }
        GetCursorSampleSet(gConfig.walkSpeed);
    }
    if (gPrefsChanged)
        WritePrefs();
    if (heroWasMobilized)
        MobilizeCurrHero(false);
    if (gameCommand != MAIN_MENU_NO_COMMAND) {
        gGameCommand = gameCommand;
        return 1;
    }
    return 0;
}

void UpdateCPanel(b8 initialDraw) {
    tag_message message;

    SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_FRAME, CONTROL_MUSIC_VOLUME);
    message.value = gConfig.musicVolume ? CPANEL_FRAME_MUSIC_ON : CPANEL_FRAME_MUSIC_OFF;
    gAdventurePanel->BroadcastMessage(message);
    message.id = CONTROL_SOUND_VOLUME;
    message.value = gConfig.soundVolume ? CPANEL_FRAME_SOUND_ON : CPANEL_FRAME_SOUND_OFF;
    gAdventurePanel->BroadcastMessage(message);
    message.id = CONTROL_WALK_SPEED;
    message.value = gConfig.walkSpeed + CPANEL_FRAME_WALK_SPEED_FIRST;
    gAdventurePanel->BroadcastMessage(message);
    message.id = CONTROL_MUSIC_SOURCE;
    message.value =
        (gConfig.musicSource ? CPANEL_MUSIC_LABEL_CD
                                                               : CPANEL_MUSIC_LABEL_LOCAL)
        + CPANEL_FRAME_MUSIC_SOURCE_FIRST;
    gAdventurePanel->BroadcastMessage(message);
    message.id = CONTROL_SHOW_ROUTE;
    message.value = gConfig.showRoute + CPANEL_FRAME_SHOW_ROUTE_FIRST;
    gAdventurePanel->BroadcastMessage(message);
    message.id = CONTROL_SHOW_ENEMY_MOVES;
    if (gRemoteOn)
        message.value = CPANEL_FRAME_ENEMY_MOVES_FIRST;
    else
        message.value = 1 - gConfig.blackoutComputer + CPANEL_FRAME_ENEMY_MOVES_FIRST;
    gAdventurePanel->BroadcastMessage(message);
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = CONTROL_MUSIC_VOLUME_TEXT;
    message.text = gOnOffText[gConfig.musicVolume];
    gAdventurePanel->BroadcastMessage(message);
    message.id = CONTROL_SOUND_VOLUME_TEXT;
    message.text = gOnOffText[gConfig.soundVolume];
    gAdventurePanel->BroadcastMessage(message);
    message.id = CONTROL_WALK_SPEED_TEXT;
    message.text = gWalkSpeedText[gConfig.walkSpeed];
    gAdventurePanel->BroadcastMessage(message);
    message.id = CONTROL_MUSIC_SOURCE_TEXT;
    message.text = gMusicQualityText
        [gConfig.musicSource ? CPANEL_MUSIC_LABEL_CD
                                                               : CPANEL_MUSIC_LABEL_LOCAL];
    gAdventurePanel->BroadcastMessage(message);
    message.id = CONTROL_SHOW_ROUTE_TEXT;
    message.text = gOnOffText[gConfig.showRoute];
    gAdventurePanel->BroadcastMessage(message);
    message.id = CONTROL_SHOW_ENEMY_MOVES_TEXT;
    message.text = gOnOffText[1 - gConfig.blackoutComputer];
    gAdventurePanel->BroadcastMessage(message);
    if (!initialDraw)
        gAdventurePanel->MoveWindow(0, 0);
}

i8 SaveGame(void) {
    i16 res;
    fileRequester* newFileReq;
    char searchMask[16];
    i8 success;
    i32 humans;
    i32 player;
    char extensionBuf[8];

    success = 0;
    humans = 0;
    gAdvManager->DisableButtons();
    gMouseManager->SetPointer("advmice.mse", ADVENTURE_POINTER_DEFAULT);
    for (player = 0; player < GAME_PLAYER_COUNT; player++)
        if (!gGame->m_playerDead[player] && gHumanPlayer[player])
            humans++;
    if (gCampaignChoice > CAMPAIGN_NONE) {
        sprintf(extensionBuf, ".CGM");
        sprintf(searchMask, "*.CGM");
    } else {
        sprintf(extensionBuf, ".GM%d", humans);
        sprintf(searchMask, "*.GM*");
    }
    newFileReq =
        new fileRequester(0xa0, 0x28, FILE_REQUESTER_SAVE, searchMask, gGamePath, extensionBuf);
    if (!newFileReq)
        MemError();
    res = gExec->DoDialog(newFileReq);
    if (res == DIALOG_BUTTON_2) {
        success = 1;
        gFreshSave = true;
        success = gGame->SaveGame(gLastFilename, false);
        if (success)
            NormalDialog(localization::Tr("adventure.save.success"), NORMAL_DIALOG_TYPE_OK, 0xb1);
    }
    delete newFileReq;
    gAdvManager->EnableButtons();
    return success;
}

i16 CPanelHandler(struct tag_message& message) {
    b8 anyChanged = false;
    char question[120];
    b8 handled = false;
    if (message.type == MESSAGE_WIDGET) {
        if (message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON) {
            if (IS_WIDGET_SELECTION_NOTIFICATION(message.command)) {
                i32 helpIndex = CPANEL_HELP_NONE;
                switch (message.id) {
                    case CONTROL_NEW_GAME:
                        helpIndex = CPANEL_HELP_NEW_GAME;
                        break;
                    case CONTROL_LOAD_GAME:
                        helpIndex = CPANEL_HELP_LOAD_GAME;
                        break;
                    case CONTROL_QUIT:
                        helpIndex = CPANEL_HELP_QUIT;
                        break;
                    case PANEL_CLOSE_WIDGET:
                        helpIndex = CPANEL_HELP_CLOSE;
                        break;
                    case CONTROL_SAVE_GAME:
                        helpIndex = CPANEL_HELP_SAVE_GAME;
                        break;
                    case CONTROL_MUSIC_VOLUME:
                        helpIndex = CPANEL_HELP_MUSIC_VOLUME;
                        break;
                    case CONTROL_SOUND_VOLUME:
                        helpIndex = CPANEL_HELP_SOUND_VOLUME;
                        break;
                    case CONTROL_WALK_SPEED:
                        helpIndex = CPANEL_HELP_WALK_SPEED;
                        break;
                    case CONTROL_MUSIC_SOURCE:
                        helpIndex = CPANEL_HELP_MUSIC_SOURCE;
                        break;
                    case CONTROL_SHOW_ROUTE:
                        helpIndex = CPANEL_HELP_SHOW_ROUTE;
                        break;
                    case CONTROL_SHOW_ENEMY_MOVES:
                        helpIndex = CPANEL_HELP_SHOW_ENEMY_MOVES;
                        break;
                    case CONTROL_SCENARIO_INFO:
                        helpIndex = CPANEL_HELP_SCENARIO_INFO;
                        break;
                }
                if (helpIndex >= CPANEL_HELP_FIRST)
                    NormalDialog(gCPanelHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW, 0xb1);
            }
        } else {
            switch (message.command) {
                case WIDGET_NOTIFY_DESELECT:
                    switch (message.id) {
                        case CONTROL_NEW_GAME:
                            strcpy(question, localization::Tr("adventure.confirm_restart"));
                            goto confirm_reset;
                        case CONTROL_LOAD_GAME:
                            strcpy(question, localization::Tr("adventure.confirm_load"));
                            goto confirm_reset;
                        case CONTROL_QUIT:
                            strcpy(question, localization::Tr("adventure.confirm_quit"));
                        confirm_reset:
                            handled = true;
                            if (!gFreshSave) {
                                NormalDialog(question, NORMAL_DIALOG_TYPE_YES_NO, 0xb1, 0x50);
                                if (gWindowManager->m_dialogResult == NORMAL_DIALOG_CANCEL)
                                    handled = false;
                            }
                            break;
                        case CONTROL_SAVE_GAME:
                            handled = true;
                            break;
                        case CONTROL_SCENARIO_INFO:
                        case PANEL_CLOSE_WIDGET:
                            handled = true;
                            break;
                    }
                    break;
                case WIDGET_NOTIFY_SELECT:
                    switch (message.id) {
                        case CONTROL_MUSIC_VOLUME:
                            gConfig.musicVolume =
                                (gConfig.musicVolume + 1) % (SOUND_VOLUME_LAST + 1);
                            SetMusicVolume(gConfig.musicVolume);
                            anyChanged = true;
                            gPrefsChanged = true;
                            break;
                        case CONTROL_SOUND_VOLUME:
                            gConfig.soundVolume =
                                (gConfig.soundVolume + 1) % (SOUND_VOLUME_LAST + 1);
                            SetEffectsVolume(gConfig.soundVolume);
                            anyChanged = true;
                            gPrefsChanged = true;
                            break;
                        case CONTROL_WALK_SPEED:
                            ++gConfig.walkSpeed;
                            WALK_SPEED_WRAP(gConfig.walkSpeed);
                            anyChanged = true;
                            gPrefsChanged = true;
                            break;
                        case CONTROL_MUSIC_SOURCE:
                            if (gConfig.musicSource) {
                                gConfig.musicSource = SOUND_MUSIC_SOURCE_DIGITAL;
                            } else {
                                gConfig.musicSource = SOUND_MUSIC_SOURCE_CD;
                            }
                            SetMusicSource(gConfig.musicSource != SOUND_MUSIC_SOURCE_DIGITAL);
                            anyChanged = true;
                            gPrefsChanged = true;
                            break;
                        case CONTROL_SHOW_ROUTE:
                            gConfig.showRoute = 1 - gConfig.showRoute;
                            anyChanged = true;
                            gPrefsChanged = true;
                            break;
                        case CONTROL_SHOW_ENEMY_MOVES:
                            if (!gRemoteOn) {
                                gConfig.blackoutComputer = 1 - gConfig.blackoutComputer;
                                anyChanged = true;
                                gPrefsChanged = true;
                            }
                            break;
                    }
                    break;
            }
        }
    }
    if (anyChanged)
        UpdateCPanel(false);
    if (handled) {
        FINISH_DIALOG_MESSAGE(message);
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

void advManager::CheckCastSpell(void) {
    if (gCurPlayerData->CurrentHero() != HERO_ID_NONE) {
        MobilizeCurrHero(false);
        CompleteDraw(false);
        UpdateScreen(false, false);
        GrabScreen();
        gMouseManager->SetPointer("advmice.mse", ADVENTURE_POINTER_DEFAULT);
        CastSpell(gGame->ViewSpells(
            gGame->GetHero(gCurPlayerData->m_currentHero),
            SPELL_TYPE_ADVENTURE,
            NullHandler,
            0
        ));
    }
}

void advManager::AdvPanel(void) {
    heroWindow* adventurePanel;
    {
        TrimLoopingSounds(ADVMGR_ACTIVE_SOUND_COUNT);
        gMouseManager->SetPointer("advmice.mse", ADVENTURE_POINTER_DEFAULT);
        b32 heroWasMobilized = m_heroContextLocked;
        struct tag_message message;
        DemobilizeCurrHero();

        adventurePanel = new heroWindow(160, 40, "apanel.bin");
        if (adventurePanel == NULL)
            MemError();
        if (gCurPlayerData->CurrentHero() == HERO_ID_NONE) {
            message.type = MESSAGE_WIDGET;
            message.id = PANEL_SEARCH;
            message.command = WIDGET_COMMAND_CLEAR_FLAGS;
            message.value = WIDGET_FLAG_ENABLED;
            adventurePanel->BroadcastMessage(message);
            message.id = PANEL_CAST_SPELL;
            adventurePanel->BroadcastMessage(message);
            message.id = PANEL_SEARCH;
            message.command = WIDGET_COMMAND_SET_FLAGS;
            message.value = WIDGET_FLAG_DIM_REQUEST;
            adventurePanel->BroadcastMessage(message);
            message.id = PANEL_CAST_SPELL;
            adventurePanel->BroadcastMessage(message);
        }

        gWindowManager->DoDialog(adventurePanel, APanelHandler, false);
        delete adventurePanel;
        switch (gWindowManager->m_dialogResult) {
            case PANEL_CAST_SPELL:
                CheckCastSpell();
                break;
            case PANEL_SEARCH:
                ProcessSearch(ADVMGR_SEARCH_VIEW_CENTER, ADVMGR_SEARCH_VIEW_CENTER);
                break;
            case PANEL_VIEW_WORLD:
                ViewWorld(SPELL_VIEW_ALL, false, false);
                break;
            case PANEL_VIEW_PUZZLE:
                ViewPuzzle();
                break;
        }

        if (heroWasMobilized)
            MobilizeCurrHero(false);
    }
}

i16 APanelHandler(struct tag_message& message) {
    b8 handled = false;
    if (message.type == MESSAGE_WIDGET) {
        if (message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON) {
            if (IS_WIDGET_SELECTION_NOTIFICATION(message.command)) {
                i32 helpIndex = PANEL_NO_HELP;
                switch (message.id) {
                    case PANEL_VIEW_WORLD:
                        helpIndex = PANEL_VIEW_WORLD_HELP;
                        break;
                    case PANEL_VIEW_PUZZLE:
                        helpIndex = PANEL_VIEW_PUZZLE_HELP;
                        break;
                    case PANEL_CAST_SPELL:
                        helpIndex = PANEL_CAST_SPELL_HELP;
                        break;
                    case PANEL_SEARCH:
                        helpIndex = PANEL_SEARCH_HELP;
                        break;
                    case PANEL_CLOSE_WIDGET:
                        helpIndex = PANEL_CLOSE_HELP;
                        break;
                }
                if (helpIndex >= 0)
                    NormalDialog(gAPanelHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW, 0xb1);
            }
        } else {
            switch (message.command) {
                case WIDGET_NOTIFY_DESELECT:
                    switch (message.id) {
                        case PANEL_VIEW_WORLD:
                        case PANEL_VIEW_PUZZLE:
                        case PANEL_CAST_SPELL:
                        case PANEL_SEARCH:
                        case PANEL_CLOSE_WIDGET:
                            handled = true;
                            break;
                    }
                    break;
                default:
                    break;
            }
        }
    }

    if (handled) {
        FINISH_DIALOG_MESSAGE(message);
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

i16 DimensionDoorHandler(struct tag_message& message) {
    b8 result;
    i16 mouseX;
    i16 mouseY;
    mapCell* cell;

    if (gTimers[ADVENTURE_FRAME_TIMER_SLOT] < KBTickCount()) {
        gAdvManager->CompleteDraw(gAdvManager->m_mapOriginX, gAdvManager->m_mapOriginY, false);
        gAdvManager->UpdateScreen(false, false);
    }
    result = false;
    switch (message.type) {
        case MESSAGE_WIDGET:
            switch (message.command) {
                case WIDGET_NOTIFY_SELECT:
                    switch (message.id) {
                        case DIMENSION_DOOR_FIRST_BUTTON:
                        case DIMENSION_DOOR_LAST_BUTTON:
                            if (message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON) {
                            } else if (gWindowManager->m_dialogResult == TRAVEL_DIALOG_ACCEPT) {
                                result = true;
                            }
                            break;
                        default:
                            break;
                    }
                    break;
                case WIDGET_COMMAND_HOVER:
                    switch (message.id) {
                        case DIMENSION_DOOR_LAST_BUTTON:
                            gWindowManager->m_dialogResult = TRAVEL_DIALOG_REJECT;
                            gMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
                            break;
                        case ADVENTURE_CONTROL_MAP_VIEW:
                            gMouseManager->MouseCoords(mouseX, mouseY);
                            mouseX /= CELL_PIXELS;
                            mouseY /= CELL_PIXELS;
                            if (mouseX < 0)
                                mouseX = 0;
                            if (mouseY < 0)
                                mouseY = 0;
                            if (mouseX > ADVMGR_VIEW_CELL_COUNT - 1)
                                mouseX = ADVMGR_VIEW_CELL_COUNT - 1;
                            if (mouseY > ADVMGR_VIEW_CELL_COUNT - 1)
                                mouseY = ADVMGR_VIEW_CELL_COUNT - 1;
                            if (gAdvManager->m_hoverCellX != mouseX
                                || gAdvManager->m_hoverCellY != mouseY) {
                                gAdvManager->m_hoverCellX = mouseX;
                                gAdvManager->m_hoverCellY = mouseY;
                                cell = gAdvManager->GetCell(
                                    gAdvManager->m_mapOriginX + mouseX,
                                    gAdvManager->m_mapOriginY + mouseY
                                );
                                if ((cell->m_triggerType & MAP_TRIGGER_EVENT)
                                    || (cell->m_secondaryTrigger & MAP_CELL_SECONDARY_BLOCKED)) {
                                    gWindowManager->m_dialogResult = TRAVEL_DIALOG_REJECT;
                                    gMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
                                } else {
                                    gWindowManager->m_dialogResult = TRAVEL_DIALOG_ACCEPT;
                                    gMouseManager->SetPointer(ADVENTURE_POINTER_MOVE);
                                }
                            }
                            break;
                        default:
                            break;
                    }
                    break;
                case WIDGET_NOTIFY_DESELECT:
                    switch (message.id) {
                        case PANEL_CLOSE_WIDGET:
                            gWindowManager->m_dialogResult = TRAVEL_DIALOG_REJECT;
                            result = true;
                            break;
                    }
                    break;
            }
            break;
    }
    if (result) {
        message.command = (message.id = WIDGET_COMMAND_DIALOG_SELECT);
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

b8 advManager::ComboDraw(i16 originX, i16 originY, b8 animate) {
    static i32 gFrameCount = 0;
    i32 updateCount;
    i32 drawY;
    i32 drawX;
    mapCell* cell;

    PollSound();
    if (!gShowIt)
        return false;
    if (m_forceCompleteDraw) {
        CompleteDraw(originX, originY, false);
        return true;
    }
    if (animate) {
        gFrameCount += gFrameStep;
        if (gFrameCount < COMBO_FRAME_LIMIT) {
            Process1WindowsMessage();
            if (gTimers[ADVENTURE_FRAME_TIMER_SLOT] < KBTickCount())
                gTimers[ADVENTURE_FRAME_TIMER_SLOT] = KBTickCount() + TIMER_DELAY;
            PollSound();
            return false;
        } else {
            gFrameCount = 0;
        }
    }

    m_previousOriginX = m_mapOriginX;
    m_previousOriginY = m_mapOriginY;
    memset(gComboDraw, 0, COMBO_CLEAR_BYTES);
    m_comboHeroDrawn = false;

    for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++) {
        for (drawY = 0; drawY < ADVMGR_VIEW_CELL_COUNT; drawY++) {
            if (originX + drawX >= 0 && originX + drawX < MAP_CELL_GRID_SIZE && originY + drawY >= 0
                && originY + drawY < MAP_CELL_GRID_SIZE) {
                cell = GetCell(originX + drawX, originY + drawY);
                if (cell->m_flags & (MAP_CELL_OBJECT_ANIMATED | MAP_CELL_OVERLAY_ANIMATED))
                    ++gComboDraw[drawX][drawY];
                if (cell->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_MONSTER)) {
                    ++gComboDraw[drawX][drawY];
                    if (GetCloudLookup(drawX + originX, drawY + originY)) {
                        gComboDraw[drawX + 1][drawY] += COMBO_CLOUD_MARK;
                        if (drawY >= 1) {
                            gComboDraw[drawX][drawY - 1] += COMBO_CLOUD_MARK;
                            gComboDraw[drawX + 1][drawY - 1] += COMBO_CLOUD_MARK;
                        }
                    } else {
                        ++gComboDraw[drawX + 1][drawY];
                        if (drawY >= 1) {
                            ++*(gComboDraw[drawX] + drawY - 1);
                            ++gComboDraw[drawX + 1][drawY - 1];
                        }
                    }
                }
                if (cell->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_HERO)
                    || cell->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_SHIP)) {
                    ++gComboDraw[drawX][drawY];
                    if (GetCloudLookup(drawX + originX, drawY + originY)) {
                        gComboDraw[drawX + 1][drawY] += COMBO_CLOUD_MARK;
                        gComboDraw[drawX][drawY + 1] += COMBO_CLOUD_MARK;
                        if (drawY >= 1)
                            gComboDraw[drawX][drawY - 1] += COMBO_CLOUD_MARK;
                        if (drawX >= 1)
                            gComboDraw[drawX - 1][drawY] += COMBO_CLOUD_MARK;
                    } else {
                        ++gComboDraw[drawX + 1][drawY];
                        ++gComboDraw[drawX][drawY + 1];
                        if (drawY >= 1)
                            ++*(gComboDraw[drawX] + drawY - 1);
                        if (drawX >= 1)
                            ++gComboDraw[drawX - 1][drawY];
                    }
                }
            }
        }
    }

    for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++) {
        for (drawY = 0; drawY < ADVMGR_VIEW_CELL_COUNT; drawY++) {
            if (gComboDraw[drawX][drawY]) {
                if (originX + drawX < 0 || originX + drawX >= MAP_CELL_GRID_SIZE
                    || originY + drawY < 0 || originY + drawY >= MAP_CELL_GRID_SIZE)
                    gComboDraw[drawX][drawY] = 0;
                else if (gComboDraw[drawX][drawY] < COMBO_CLOUD_MARK
                         && !GetCloudLookup(drawX + originX, drawY + originY))
                    gComboDraw[drawX][drawY] = 0;
            }
        }
    }

    if (gMouseManager->IsVis()) {
        drawX = gMouseManager->m_savedLeft >> CELL_PIXEL_SHIFT;
        drawY = gMouseManager->m_savedTop >> CELL_PIXEL_SHIFT;
        ++gComboDraw[drawX][drawY];
        ++gComboDraw[drawX + 1][drawY];
        ++gComboDraw[drawX][drawY + 1];
        ++gComboDraw[drawX + 1][drawY + 1];
        ++gComboDraw[drawX + COMBO_FAR_NEIGHBOR_OFFSET][drawY + 1];
    }
    if (m_heroContextLocked) {
        for (drawY = ADVMGR_VIEW_CENTER - 1; drawY <= ADVMGR_VIEW_CENTER + 1; drawY++)
            for (drawX = ADVMGR_VIEW_CENTER - 1; drawX <= ADVMGR_VIEW_CENTER + 1; drawX++)
                ++gComboDraw[drawX][drawY];
    }
    if (m_cursorType == ADVMGR_HERO_ICON_BOAT) {
        ++gComboDraw[ADVMGR_VIEW_CENTER - 1][ADVMGR_VIEW_CENTER - 2];
        ++gComboDraw[ADVMGR_VIEW_CENTER][ADVMGR_VIEW_CENTER - 2];
        ++gComboDraw[ADVMGR_VIEW_CENTER + 1][ADVMGR_VIEW_CENTER - 2];
    }

    for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++) {
        if (gComboDraw[drawX][0])
            DrawCell(
                originX + drawX,
                originY,
                drawX,
                0,
                ADVMGR_DRAW_GROUND | ADVMGR_DRAW_OBJECT,
                false,
                false
            );
    }
    for (drawY = 1; drawY < ADVMGR_VIEW_CELL_COUNT; drawY++) {
        for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++) {
            if (gComboDraw[drawX][drawY])
                DrawCell(
                    originX + drawX,
                    originY + drawY,
                    drawX,
                    drawY,
                    ADVMGR_DRAW_GROUND,
                    false,
                    false
                );
        }
    }
    for (drawY = 1; drawY < ADVMGR_VIEW_CELL_COUNT; drawY++) {
        PollSound();
        for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++) {
            if (gComboDraw[drawX][drawY - 1])
                DrawCell(
                    originX + drawX,
                    originY + drawY - 1,
                    drawX,
                    drawY - 1,
                    ADVMGR_DRAW_OVERLAY | ADVMGR_DRAW_HERO,
                    false,
                    false
                );
        }
        for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++) {
            if (gComboDraw[drawX][drawY])
                DrawCell(
                    originX + drawX,
                    originY + drawY,
                    drawX,
                    drawY,
                    ADVMGR_DRAW_OBJECT,
                    false,
                    false
                );
        }
    }
    for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++) {
        if (gComboDraw[drawX][ADVMGR_VIEW_CELL_COUNT - 1])
            DrawCell(
                originX + drawX,
                originY + ADVMGR_VIEW_CELL_COUNT - 1,
                drawX,
                ADVMGR_VIEW_CELL_COUNT - 1,
                ADVMGR_DRAW_OVERLAY | ADVMGR_DRAW_HERO,
                false,
                false
            );
    }
    for (drawY = 0; drawY < ADVMGR_VIEW_CELL_COUNT; drawY++) {
        for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++) {
            if (gComboDraw[drawX][drawY])
                DrawCell(
                    originX + drawX,
                    originY + drawY,
                    drawX,
                    drawY,
                    ADVMGR_DRAW_CLOUD,
                    false,
                    false
                );
        }
    }

    PollSound();
    UpdBottomView(false, true, true);
    DrawAdventureBorder();
    gLimitUpdMinX = ADVMGR_VIEW_CELL_COUNT;
    gLimitUpdMinY = ADVMGR_VIEW_CELL_COUNT;
    gLimitUpdMaxX = 0;
    gLimitUpdMaxY = 0;
    updateCount = 0;
    for (drawY = 0; drawY < ADVMGR_VIEW_CELL_COUNT; drawY++) {
        for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++) {
            if (gComboDraw[drawX][drawY]) {
                updateCount++;
                if (drawX < gLimitUpdMinX)
                    gLimitUpdMinX = drawX;
                if (drawX > gLimitUpdMaxX)
                    gLimitUpdMaxX = drawX;
                if (drawY < gLimitUpdMinY)
                    gLimitUpdMinY = drawY;
                if (drawY > gLimitUpdMaxY)
                    gLimitUpdMaxY = drawY;
            }
        }
    }
    gLimitUpdMinX <<= CELL_PIXEL_SHIFT;
    gLimitUpdMinY <<= CELL_PIXEL_SHIFT;
    gLimitUpdMaxX = ((gLimitUpdMaxX + 1) << CELL_PIXEL_SHIFT) - 1;
    gLimitUpdMaxY = ((gLimitUpdMaxY + 1) << CELL_PIXEL_SHIFT) - 1;
    if (gLimitUpdMinX < BORDER_EDGE_SIZE)
        gLimitUpdMinX = BORDER_EDGE_SIZE;
    if (gLimitUpdMaxX > BORDER_MIDDLE_END - 1)
        gLimitUpdMaxX = BORDER_MIDDLE_END - 1;
    if (gLimitUpdMinY < BORDER_EDGE_SIZE)
        gLimitUpdMinY = BORDER_EDGE_SIZE;
    if (gLimitUpdMaxY > BORDER_MIDDLE_END - 1)
        gLimitUpdMaxY = BORDER_MIDDLE_END - 1;
    if (gLimitUpdMinX > gLimitUpdMaxX || gLimitUpdMinY > gLimitUpdMaxY) {
        gLimitUpdMinX = gLimitUpdMaxX - 1;
        gLimitUpdMinY = gLimitUpdMaxY - 1;
        return false;
    }
    return true;
}

b8 advManager::ComboDraw(b32 animate) {
    return ComboDraw(m_mapOriginX, m_mapOriginY, animate);
}

void advManager::SetEnvironmentOrigin(i16 originX, i16 originY, i16 stopSounds) {
    i32 soundRadius;
    i32 edgeOffset;
    i32 maxCells = ADVMGR_ACTIVE_SOUND_COUNT / 2;
    i32 layer;

    for (edgeOffset = 0; edgeOffset < ADVMGR_ACTIVE_SOUND_COUNT; ++edgeOffset) {
        if (m_activeSounds[edgeOffset].soundId != MAP_SOUND_NONE) {
            if (stopSounds) {
                StopSample(m_loopingSamples[m_activeSounds[edgeOffset].soundId]);
                m_activeSounds[edgeOffset].soundId = MAP_SOUND_NONE;
                m_activeSounds[edgeOffset].distance = ENVIRONMENT_SOUND_FAR_DISTANCE;
            } else {
                m_activeSounds[edgeOffset].distance = ENVIRONMENT_SOUND_FAR_DISTANCE;
            }
        }
    }
    if (gConfig.soundVolume != SOUND_VOLUME_OFF) {
        m_activeSoundMask = 0;
        for (layer = ENVIRONMENT_SOUND_FIRST_LAYER; layer <= ENVIRONMENT_SOUND_LAYER_COUNT;
             ++layer) {
            InsertSound(originX, originY, 0, layer);
            for (soundRadius = 0; soundRadius < ENVIRONMENT_SOUND_RADIUS_COUNT; ++soundRadius) {
                for (edgeOffset = 0; edgeOffset < soundRadius * ENVIRONMENT_SOUND_EDGE_SPAN;
                     ++edgeOffset) {
                    InsertSound(
                        originX - soundRadius + edgeOffset,
                        originY - soundRadius,
                        soundRadius,
                        layer
                    );
                    InsertSound(
                        originX + soundRadius,
                        originY - soundRadius + edgeOffset,
                        soundRadius,
                        layer
                    );
                    InsertSound(
                        originX + soundRadius - edgeOffset,
                        originY + soundRadius,
                        soundRadius,
                        layer
                    );
                    InsertSound(
                        originX - soundRadius,
                        originY + soundRadius - edgeOffset,
                        soundRadius,
                        layer
                    );
                }
            }
        }
        for (edgeOffset = 0; edgeOffset < ADVMGR_ACTIVE_SOUND_COUNT; ++edgeOffset) {
            if (m_activeSounds[edgeOffset].soundId != MAP_SOUND_NONE
                && m_activeSounds[edgeOffset].distance > ENVIRONMENT_SOUND_MAX_DISTANCE) {
                StopSample(m_loopingSamples[m_activeSounds[edgeOffset].soundId]);
                m_activeSounds[edgeOffset].soundId = MAP_SOUND_NONE;
            }
            if (m_activeSounds[edgeOffset].soundId != MAP_SOUND_NONE
                && (m_activeSoundMask & (1 << m_activeSounds[edgeOffset].soundId)) != 0) {
                m_loopingSamples[m_activeSounds[edgeOffset].soundId]->m_playbackData.volume =
                    gEnvironmentVolume[m_activeSounds[edgeOffset].distance];
                UpdateSampleVolume(m_loopingSamples[m_activeSounds[edgeOffset].soundId]);
            }
        }
    }
}

void advManager::CheckLoadSample(i32 index) {
    if (m_loopingSamples[index] == NULL) {
        TrimLoopingSounds(ADVMGR_ACTIVE_SOUND_COUNT);
        sprintf(gText, "loop%04d.82M", index);
        m_loopingSamples[index] = gResourceManager->GetSample(gText);
    }
}

void advManager::InsertSound(i16 x, i16 y, i16 distance, i8 soundLayer) {
    i32 slot;
    i32 distanceLimit;
    i32 i;
    i32 soundId;

    if (x < 0 || y < 0 || x >= MAP_CELL_GRID_SIZE || y >= MAP_CELL_GRID_SIZE)
        return;
    soundId = gGame->m_mapSounds[x][y];
    if (soundId == MAP_SOUND_NONE)
        return;
    for (i = 0; i < ADVMGR_ACTIVE_SOUND_COUNT; ++i) {
        if (m_activeSounds[i].soundId == soundId) {
            if (m_activeSounds[i].distance > distance) {
                m_activeSounds[i].distance = distance;
                m_activeSoundMask |= 1 << m_activeSounds[i].soundId;
            }
            return;
        }
    }
    if (soundLayer == ENVIRONMENT_SOUND_FIRST_LAYER)
        return;
    distanceLimit = distance;
    slot = ENVIRONMENT_SOUND_NO_SLOT;
    for (i = 0; i < ADVMGR_ACTIVE_SOUND_COUNT; ++i) {
        if (m_activeSounds[i].distance > distanceLimit) {
            distanceLimit = m_activeSounds[i].distance;
            slot = i;
        }
    }
    if (slot != ENVIRONMENT_SOUND_NO_SLOT) {
        if (m_activeSounds[slot].soundId != MAP_SOUND_NONE)
            StopSample(m_loopingSamples[m_activeSounds[slot].soundId]);
        m_activeSounds[slot].soundId = soundId;
        m_activeSounds[slot].distance = distance;
        CheckLoadSample(soundId);
        m_loopingSamples[soundId]->m_playbackData.volume = gEnvironmentVolume[distance];
        m_loopingSamples[soundId]->m_playbackData.repeat = true;
        PlaySample(m_loopingSamples[soundId]);
        m_activeSoundMask ^= 1 << m_activeSounds[slot].soundId;
    }
}

i32 gThisMaxY;
i32 gThisMinY;
i32 gUnusedAdvTemp;
i32 gAdvSpareFlag2;
i32 gAdvSpareInt;

void advManager::TeleportTo(i32 x, i32 y, i32) {
    b32 savedShow;
    i32 curFizzle;
    mapCell* location;
    mapCell* savedOldCell;
    i32 hold;
    i8 newTerrain;
    hero* mapHero;
    town* occupiedTown;

    savedShow = gShowIt;
    mapHero = gGame->GetHero(gCurPlayerData->m_currentHero);
    location = GetCell(x, y);
    savedOldCell = GetCell(m_mapOriginX + ADVMGR_VIEW_CENTER, m_mapOriginY + ADVMGR_VIEW_CENTER);
    if (mapHero->m_locationType == MAP_EVENT_TRIGGER(MAP_OBJECT_TOWN)) {
        occupiedTown = gGame->GetTown(mapHero->m_occupiedTown);
        occupiedTown->m_occupyingHeroId = TOWN_OCCUPYING_HERO_NONE;
    }
    if (savedOldCell->m_flags & MAP_CELL_HERO_CURSOR)
        MAP_CELL_SUBTRACT_FLAG(savedOldCell->m_flags, MAP_CELL_HERO_CURSOR);
    CompleteDraw(false);
    if (!gHumanPlayer[gCurPlayer]) {
        if (!gConfig.blackoutComputer && !gRemoteOn
            && (gGame->m_mapExtra[mapHero->m_x][mapHero->m_y] & gCurWatchPlayerHighBit))
            gShowIt = true;
        else
            gShowIt = false;
    }
    if (savedShow)
        HideRoute(true, true, true);
    // The current hero's position is the view's centre (DemobilizeCurrHero
    // takes it from there), and MoveHero moves the view with the hero
    // whether or not the move is shown. The original moved the view here
    // only when the teleport was shown: a computer player's Dimension Door
    // out of the watcher's sight was undone when the hero was put down,
    // after the spell and the movement were spent, and the hero could be
    // put down onto a boat or another hero, leaving a hero cell behind.
    m_mapOriginX = x - ADVMGR_VIEW_CENTER;
    m_mapOriginY = y - ADVMGR_VIEW_CENTER;
    if (gShowIt)
        DelayMilli(90);
    mapHero->m_x = x;
    mapHero->m_y = y;
    gGame->SetVisibility(
        m_mapOriginX + ADVMGR_VIEW_CENTER,
        m_mapOriginY + ADVMGR_VIEW_CENTER,
        gCurPlayer,
        gHeroScoutRadius[mapHero->m_heroClass]
    );
    if (gShowIt) {
        location->m_flags |= MAP_CELL_HERO_CURSOR;
        gMouseManager->ReallyHidePointer();
        gWindowManager->SaveFizzleSource(
            BORDER_EDGE_SIZE,
            BORDER_EDGE_SIZE,
            ADVENTURE_VIEWPORT_INNER_SIZE,
            ADVENTURE_VIEWPORT_INNER_SIZE
        );
        CompleteDraw(false);
        PollSound();
        curFizzle = TELEPORT_FIZZLE_TIME;
        if (!gHumanPlayer[gCurPlayer])
            curFizzle -= TELEPORT_REMOTE_FIZZLE_ADJUSTMENT;
        gWindowManager->FizzleForward(
            BORDER_EDGE_SIZE,
            BORDER_EDGE_SIZE,
            ADVENTURE_VIEWPORT_INNER_SIZE,
            ADVENTURE_VIEWPORT_INNER_SIZE,
            FIZZLE_USE_DEFAULT_DELAY
        );
        PollSound();
        gMouseManager->ReallyShowPointer();
    }
    SetEnvironmentOrigin(m_mapOriginX + ADVMGR_VIEW_CENTER, m_mapOriginY + ADVMGR_VIEW_CENTER, 1);
    newTerrain = CELL_TERRAIN(location);
    if (newTerrain != m_currentTerrain) {
        m_currentTerrain = newTerrain;
        PlayMusic(TERRAIN_MUSIC_TRACK(m_currentTerrain));
    }
    Reseed(0, 0);
    UpdateRadar(true, false);
    CompleteDraw(false);
    ForceNewHover();
}

void advManager::DimensionDoor(void) {
    hero* targetHero;
    heroWindow* window;
    i16 newX;
    i16 newY;
    mapCell* targetCell;

    window = new heroWindow(0, 0, "dimdoor.bin");
    if (window == NULL)
        MemError();
    SetWinText(window, WINDOW_TEXT_DIMENSION_DOOR);
    gWindowManager->DoDialog(window, DimensionDoorHandler, false);
    delete window;
    targetHero = gGame->GetHero(gCurPlayerData->m_currentHero);
    if (gWindowManager->m_dialogResult == TRAVEL_DIALOG_ACCEPT) {
        newX = m_mapOriginX + m_hoverCellX;
        newY = m_mapOriginY + m_hoverCellY;
        targetCell = GetCell(newX, newY);
        // The view can show the border beyond the map's edge. The original
        // tested the clamped cell (0,0) for such a target and teleported the
        // hero off the map (Tournament Edition X28); it now fails.
        if (newX < 0 || newY < 0 || newX >= MAP_CELL_GRID_SIZE || newY >= MAP_CELL_GRID_SIZE
            || (targetHero->IsEmbarked() && targetCell->m_tileIndex >= MAP_CELL_TILES_PER_TERRAIN)
            || (!targetHero->IsEmbarked()
                && targetCell->m_tileIndex < MAP_CELL_TILES_PER_TERRAIN)) {
            NormalDialog(
                localization::Tr("adventure.dimension_door.failed"),
                NORMAL_DIALOG_TYPE_OK,
                0x61,
                0x91
            );
            UpdateRadar(true, false);
        } else {
            PlayMusic(MUSIC_TRACK_TELEPORT);
            TeleportTo(newX, newY, 0);
            PlayMusic(TERRAIN_MUSIC_TRACK(m_currentTerrain));
        }
        gGame->GetHero(gCurPlayerData->m_currentHero)->UseSpell(SPELL_DIMENSION_DOOR);
    } else {
        UpdateRadar(true, false);
    }
}

struct tag_message gUSMsg;
struct tag_message gCDMsg;

void advManager::TownGate(void) {
    hero* targetHero;
    i32 dist;
    i32 selectedTown;
    i32 i;
    i32 nearestDistance;

    nearestDistance = TOWN_PORTAL_DISTANCE_LIMIT;
    selectedTown = TOWN_GATE_NO_TOWN;
    targetHero = gGame->GetHero(gCurPlayerData->m_currentHero);
    if (targetHero->IsEmbarked()) {
        NormalDialog(localization::Tr("adventure.town_gate.land_required"), NORMAL_DIALOG_TYPE_OK);
        return;
    }
    for (i = 0; i < gCurPlayerData->m_townCount; i++) {
        dist = MANHATTAN_LENGTH(
            gGame->m_castleRecs[gCurPlayerData->m_townIds[i]].m_x - targetHero->m_x,
            gGame->m_castleRecs[gCurPlayerData->m_townIds[i]].m_y - targetHero->m_y
        );
        if (dist < nearestDistance) {
            nearestDistance = dist;
            selectedTown = i;
        }
    }
    // The original went on after this message and teleported the hero into
    // the town named by the byte before the town list.
    if (selectedTown == TOWN_GATE_NO_TOWN) {
        NormalDialog(localization::Tr("adventure.town_gate.no_town"), NORMAL_DIALOG_TYPE_OK);
        return;
    }
    if (gGame->m_castleRecs[gCurPlayerData->m_townIds[selectedTown]].m_occupyingHeroId
        != TOWN_OCCUPYING_HERO_NONE) {
        NormalDialog(localization::Tr("adventure.town_gate.occupied"), NORMAL_DIALOG_TYPE_OK, 0x61);
        return;
    }
    PlayMusic(MUSIC_TRACK_TELEPORT);
    TeleportTo(
        gGame->m_castleRecs[gCurPlayerData->m_townIds[selectedTown]].m_x,
        gGame->m_castleRecs[gCurPlayerData->m_townIds[selectedTown]].m_y,
        0
    );
    targetHero->UseSpell(SPELL_TOWN_GATE);
    gGame->m_castleRecs[gCurPlayerData->m_townIds[selectedTown]].m_occupyingHeroId =
        targetHero->m_id;
    gGame->m_castleRecs[gCurPlayerData->m_townIds[selectedTown]].GiveSpells();
    targetHero->m_locationType = MAP_EVENT_TRIGGER(MAP_OBJECT_TOWN);
    targetHero->m_occupiedTown = gCurPlayerData->m_townIds[selectedTown];
    PlayMusic(TERRAIN_MUSIC_TRACK(m_currentTerrain));
}

void advManager::SummonBoat(void) {
    hero* summonHero;
    mapCell* destinationCell;
    b8 okCell;
    i16 slotIndex;
    i16 iDir;
    b8 foundBoat;
    i8 heroSlot;
    boatRecord* boatRec;
    mapCell* fromCell;
    i16 clipWidth;
    i16 clipX;
    i16 clipY;
    i16 clipHeight;
    i16 boatX;
    i16 boatY;

    summonHero = gGame->GetHero(gCurPlayerData->m_currentHero);
    okCell = false;
    foundBoat = false;
    destinationCell = GetCell(m_mapOriginX + ADVMGR_VIEW_CENTER, m_mapOriginY + ADVMGR_VIEW_CENTER);
    if (destinationCell->m_tileIndex < MAP_CELL_TILES_PER_TERRAIN)
        goto summon_done;
    for (iDir = MAP_DIRECTION_FIRST; iDir < MAP_DIRECTION_COUNT; iDir++) {
        boatX = m_mapOriginX + gNormalDirTable[iDir].x + ADVMGR_VIEW_CENTER;
        boatY = m_mapOriginY + gNormalDirTable[iDir].y + ADVMGR_VIEW_CENTER;
        // Beside the map's edge there is no water to put the boat on.
        if (!MAP_CELL_IN_BOUNDS(boatX, boatY))
            continue;
        destinationCell = GetCell(boatX, boatY);
        if (destinationCell->m_objectIndex == MAP_CELL_NO_FRAME
            && destinationCell->m_tileIndex < MAP_CELL_TILES_PER_TERRAIN) {
            okCell = true;
            break;
        }
    }
    if (okCell) {
        heroSlot = gCurPlayerData->CurrentHero();
        for (slotIndex = 0; slotIndex < GAME_BOAT_COUNT; slotIndex++) {
            if (gGame->m_boatSlots[slotIndex] != GAME_TABLE_FREE
                && gGame->m_boats[slotIndex].heroId == (heroSlot | BOAT_OCCUPIED_FLAG)) {
                foundBoat = true;
                break;
            }
        }
        if (!foundBoat) {
            for (slotIndex = 0; slotIndex < GAME_BOAT_COUNT; slotIndex++) {
                if (gGame->m_boatSlots[slotIndex] != GAME_TABLE_FREE
                    && (gGame->m_boats[slotIndex].heroId & BOAT_OCCUPIED_FLAG)
                    && gGame->m_boats[slotIndex].owner == gCurPlayer) {
                    foundBoat = true;
                    break;
                }
            }
        }
        if (foundBoat) {
            boatRec = &gGame->m_boats[slotIndex];
            fromCell = GetCell(boatRec->x, boatRec->y);
            gGame->RestoreCell(
                boatRec->x,
                boatRec->y,
                boatRec->savedTriggerType,
                boatRec->savedEventData,
                NULL,
                SUMMON_RESTORE_MODE
            );
            if (boatRec->x >= m_mapOriginX && boatRec->x < m_mapOriginX + ADVMGR_VIEW_CELL_COUNT
                && boatRec->y >= m_mapOriginY
                && boatRec->y < m_mapOriginY + ADVMGR_VIEW_CELL_COUNT) {
                clipX = (boatRec->x - m_mapOriginX) * CELL_PIXELS - SUMMON_FIZZLE_X_OFFSET;
                if (clipX < BORDER_EDGE_SIZE)
                    clipX = BORDER_EDGE_SIZE;
                clipY = (boatRec->y - m_mapOriginY) * CELL_PIXELS - SUMMON_FIZZLE_Y_OFFSET;
                if (clipY < BORDER_EDGE_SIZE)
                    clipY = BORDER_EDGE_SIZE;
                clipWidth = SUMMON_FIZZLE_WIDTH;
                clipHeight = SUMMON_FIZZLE_HEIGHT;
                if (clipX + clipWidth >= BORDER_MIDDLE_END)
                    clipWidth = BORDER_MIDDLE_END - clipX;
                if (clipY + clipHeight >= BORDER_MIDDLE_END)
                    clipHeight = BORDER_MIDDLE_END - clipY;
                gWindowManager->SaveFizzleSource(clipX, clipY, clipWidth, clipHeight);
                CompleteDraw(m_mapOriginX, m_mapOriginY, false);
                gWindowManager
                    ->FizzleForward(clipX, clipY, clipWidth, clipHeight, FIZZLE_USE_DEFAULT_DELAY);
            }
            boatRec->x = boatX;
            boatRec->y = boatY;
            boatRec->savedTriggerType = destinationCell->m_triggerType;
            boatRec->savedEventData = destinationCell->m_objectMetadata;
            destinationCell->m_triggerType = MAP_EVENT_TRIGGER(MAP_OBJECT_SHIP);
            destinationCell->m_objectMetadata = slotIndex;
            gWindowManager->SaveFizzleSource(176, 192, 128, 96);
            CompleteDraw(m_mapOriginX, m_mapOriginY, false);
            gWindowManager->FizzleForward(
                SUMMON_TARGET_X,
                SUMMON_TARGET_Y,
                SUMMON_TARGET_WIDTH,
                SUMMON_TARGET_HEIGHT,
                FIZZLE_USE_DEFAULT_DELAY
            );
        }
    }

summon_done:
    UpdateScreen(false, false);
    Reseed(0, 0);
    if (!foundBoat)
        NormalDialog(
            localization::Tr("adventure.summon_boat.failed"),
            NORMAL_DIALOG_TYPE_OK,
            0x61,
            0x91
        );
}

void advManager::ShowRoute(b32 redraw, i32, b32 updateButton) {
    hero* hero;
    b32 reachable;
    i32 fromDir;
    i32 remain;
    i32 mapX;
    i32 index;
    i32 mapY;
    i32 dir;
    i32 terr;
    i16 flagCommand;

    reachable = false;
    if (!gThisNetHumanPlayer[gCurPlayer] && (!gDebugLevel || !gShowComputerRoute))
        return;
    if (gCurPlayerData->m_currentHero == HERO_ID_NONE) {
        HideRoute(redraw, false, true);
        return;
    }
    hero = gGame->GetHero(gCurPlayerData->m_currentHero);
    if (hero->m_destinationX == HERO_DESTINATION_NONE) {
        HideRoute(redraw, true, true);
        return;
    }
    gSearchArray->BuildPath(
        hero->m_x,
        hero->m_y,
        hero->m_destinationX,
        hero->m_destinationY,
        SEARCH_UNLIMITED_COST
    );
    if (gSearchArray->m_pathLength > 0) {
        memset(m_routeMap, 0, MAP_CELL_GRID_SIZE * MAP_CELL_GRID_SIZE);
        m_routeShown = true;
        remain = hero->m_remainingMobility;
        mapX = hero->m_x;
        mapY = hero->m_y;
        for (index = gSearchArray->m_pathLength - 1; index >= 0; --index) {
            dir = gSearchArray->m_directions[index];
            terr = CELL_TERRAIN(GetCell(mapX, mapY));
            remain -= CalcTerrainCost(
                terr,
                dir & MAP_DIRECTION_DIAGONAL_BIT,
                remain,
                hero->m_heroClass
            );
            mapX += gNormalDirTable[dir].x;
            mapY += gNormalDirTable[dir].y;
            if (index == 0) {
                m_routeMap[mapX + mapY * MAP_CELL_GRID_SIZE] = ROUTE_CELL_DESTINATION;
            } else {
                fromDir = gSearchArray->m_directions[index - 1];
                m_routeMap[mapX + mapY * MAP_CELL_GRID_SIZE] = gRouteFrame[fromDir][dir];
            }
            if (remain >= 0) {
                m_routeMap[mapX + mapY * MAP_CELL_GRID_SIZE] += ROUTE_CELL_REACHABLE_OFFSET;
                reachable = true;
            }
        }
        if (updateButton) {
            flagCommand = reachable ? WIDGET_COMMAND_CLEAR_FLAGS : WIDGET_COMMAND_SET_FLAGS;
            gWindowManager->BroadcastMessage(
                MESSAGE_WIDGET,
                flagCommand,
                ADVENTURE_CONTROL_CONTINUE_ROUTE,
                WIDGET_FLAG_UPDATE | WIDGET_FLAG_DIMMED
            );
        }
    } else {
        HideRoute(redraw, true, true);
    }
    if (redraw) {
        CompleteDraw(false);
        gMouseManager->ReallyHidePointer();
        UpdateScreen(false, false);
        gMouseManager->ReallyShowPointer();
    }
}

void advManager::HideRoute(b32 redraw, b32 clearDestination, b32 updateButton) {
    hero* currentHero;

    if (!gThisNetHumanPlayer[gCurPlayer] && (!gDebugLevel || !gShowComputerRoute))
        return;

    if (updateButton)
        gWindowManager->BroadcastMessage(
            MESSAGE_WIDGET,
            WIDGET_COMMAND_SET_FLAGS,
            PANEL_CONTINUE_ROUTE,
            WIDGET_FLAG_UPDATE | WIDGET_FLAG_DIMMED
        );

    if (clearDestination && gCurPlayerData->m_currentHero != HERO_ID_NONE) {
        currentHero = gGame->GetHero(gCurPlayerData->m_currentHero);
        currentHero->m_destinationX = HERO_DESTINATION_NONE;
        currentHero->m_destinationY = HERO_DESTINATION_NONE;
    }

    if (!m_routeShown)
        return;

    m_routeShown = false;
    if (redraw) {
        CompleteDraw(false);
        UpdateScreen(false, false);
    }
}

void advManager::CheckDimHero(void) {
    if (!gThisNetHumanPlayer[gCurPlayer] || gCurPlayerData->CurrentHero() == HERO_ID_NONE)
        return;
    if (!gGame->IsMobile(gCurPlayerData->CurrentHero())) {
        ShowRoute(true, 0, false);
        UpdateHeroLocators(true, 1);
        gAdvManager->CheckDimNextHeroBut();
    }
}

void advManager::CheckDimNextHeroBut(void) {
    i16 flagCommand;

    flagCommand = gThisNetHumanPlayer[gCurPlayer] && gCurPlayerData->HasMobileHero()
                      ? static_cast<i16>(WIDGET_COMMAND_CLEAR_FLAGS)
                      : static_cast<i16>(WIDGET_COMMAND_SET_FLAGS);
    gWindowManager->BroadcastMessage(
        MESSAGE_WIDGET,
        flagCommand,
        BUTTON_BROADCAST_ARG,
        WIDGET_FLAG_UPDATE | WIDGET_FLAG_DIMMED
    );
}

void advManager::SeedTo(i32 targetX, i32 targetY) {
    hero* currentHero;

    if (!gThisNetHumanPlayer[gCurPlayer])
        return;
    if (gCurPlayerData->m_currentHero == HERO_ID_NONE)
        return;

    currentHero = gGame->GetHero(gCurPlayerData->m_currentHero);
    if (!gSeedingValid)
        gSearchArray->SeedPosition(
            currentHero->m_x,
            currentHero->m_y,
            m_cursorDirection,
            SEARCH_UNLIMITED_COST,
            m_cursorType == ADVMGR_HERO_ICON_BOAT,
            false,
            currentHero->m_remainingMobility,
            currentHero->m_heroClass,
            targetX,
            targetY,
            false,
            true
        );
    else if (!gFullySeeded)
        gSearchArray->SeedPosition(
            currentHero->m_x,
            currentHero->m_y,
            m_cursorDirection,
            SEARCH_UNLIMITED_COST,
            m_cursorType == ADVMGR_HERO_ICON_BOAT,
            false,
            currentHero->m_remainingMobility,
            currentHero->m_heroClass,
            targetX,
            targetY,
            true,
            true
        );
}

void advManager::ForceNewHover(void) {
    struct tag_message msg;

    if (!gThisNetHumanPlayer[gCurPlayer])
        return;
    m_hoverCellX = CURSOR_INVALID_POSITION;
    msg.id = ADVENTURE_CONTROL_MAP_VIEW;
    ProcessHover(&msg);
}

void advManager::ScreenScroll(i8 direction, b32 updatePointer) {
    i16 yOrigin;
    i16 xOrigin;

    xOrigin = m_mapOriginX;
    yOrigin = m_mapOriginY;
    gLastScrollTime = KBTickCount();

    switch (direction) {
        case MAP_DIRECTION_NORTH:
            --yOrigin;
            break;
        case MAP_DIRECTION_NORTH_EAST:
            ++xOrigin;
            --yOrigin;
            break;
        case MAP_DIRECTION_EAST:
            ++xOrigin;
            break;
        case MAP_DIRECTION_SOUTH_EAST:
            ++xOrigin;
            ++yOrigin;
            break;
        case MAP_DIRECTION_SOUTH:
            ++yOrigin;
            break;
        case MAP_DIRECTION_SOUTH_WEST:
            --xOrigin;
            ++yOrigin;
            break;
        case MAP_DIRECTION_WEST:
            --xOrigin;
            break;
        case MAP_DIRECTION_NORTH_WEST:
            --xOrigin;
            --yOrigin;
            break;
    }

    if (updatePointer)
        gMouseManager->SetPointer(
            direction + HOVER_SCROLL_FRAME_FIRST
        );

    if (xOrigin < SCROLL_MIN_ORIGIN)
        xOrigin = SCROLL_MIN_ORIGIN;
    if (xOrigin > SCROLL_MAX_ORIGIN)
        xOrigin = SCROLL_MAX_ORIGIN;
    if (yOrigin < SCROLL_MIN_ORIGIN)
        yOrigin = SCROLL_MIN_ORIGIN;
    if (yOrigin > SCROLL_MAX_ORIGIN)
        yOrigin = SCROLL_MAX_ORIGIN;

    if (xOrigin != m_mapOriginX || yOrigin != m_mapOriginY) {
        DemobilizeCurrHero();
        m_mapOriginX = xOrigin;
        m_mapOriginY = yOrigin;
        UpdateRadar(true, false);
        CompleteDraw(false);
        UpdateScreen(false, false);
    }
}

void advManager::CheckScreenScroll(void) {
    i16 mouseX;
    i16 mouseY;
    i32 oldMapX;
    i32 oldMapY;

    if (KBTickCount() - gLastScrollTime > SCROLL_TICK_INTERVAL) {
        gLastScrollTime = KBTickCount();
        oldMapX = m_mapOriginX;
        oldMapY = m_mapOriginY;
        gMouseManager->MouseCoords(mouseX, mouseY);

        if (mouseX >= 0 && mouseX < LOGICAL_SCREEN_WIDTH && mouseY >= 0
            && mouseY < LOGICAL_SCREEN_HEIGHT) {
            if (mouseX < SCROLL_BORDER) {
                if (mouseY < SCROLL_BORDER)
                    ScreenScroll(MAP_DIRECTION_NORTH_WEST, true);
                else if (mouseY > LOGICAL_SCREEN_HEIGHT - SCROLL_BORDER)
                    ScreenScroll(MAP_DIRECTION_SOUTH_WEST, true);
                else
                    ScreenScroll(MAP_DIRECTION_WEST, true);
            } else if (mouseX > LOGICAL_SCREEN_WIDTH - SCROLL_BORDER - 1) {
                if (mouseY < SCROLL_BORDER)
                    ScreenScroll(MAP_DIRECTION_NORTH_EAST, true);
                else if (mouseY > LOGICAL_SCREEN_HEIGHT - SCROLL_BORDER)
                    ScreenScroll(MAP_DIRECTION_SOUTH_EAST, true);
                else
                    ScreenScroll(MAP_DIRECTION_EAST, true);
            } else if (mouseY < SCROLL_BORDER) {
                ScreenScroll(MAP_DIRECTION_NORTH, true);
            } else if (mouseY > LOGICAL_SCREEN_HEIGHT - SCROLL_BORDER) {
                ScreenScroll(MAP_DIRECTION_SOUTH, true);
            }
        }

        if (gMouseManager->m_cursorFrame >= HOVER_SCROLL_FRAME_FIRST
            && gMouseManager->m_cursorFrame < HOVER_SCROLL_FRAME_END && oldMapX == m_mapOriginX
            && oldMapY == m_mapOriginY)
            gMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
    }
}

b32 advManager::MouseInScrollZone(void) {
    i16 mouseX;
    i16 mouseY;

    gMouseManager->MouseCoords(mouseX, mouseY);
    if (mouseX >= 0 && mouseX < LOGICAL_SCREEN_WIDTH && mouseY >= 0
        && mouseY < LOGICAL_SCREEN_HEIGHT) {
        if (mouseX < SCROLL_BORDER || mouseX > LOGICAL_SCREEN_WIDTH - SCROLL_BORDER - 1
            || mouseY < SCROLL_BORDER || mouseY > LOGICAL_SCREEN_HEIGHT - SCROLL_BORDER) {
            return true;
        }
    }
    return false;
}

void advManager::SetInitialMapOrigin(void) {
    i16 x;
    i16 y;
    game* gameStateItem;
    hero* heroPtr;
    town* townPointer;
    town* firstTownPointer;

    gWindowManager->BroadcastMessage(
        MESSAGE_WIDGET,
        WIDGET_COMMAND_SET_FLAGS,
        ADVENTURE_CONTROL_CONTINUE_ROUTE,
        WIDGET_FLAG_UPDATE | WIDGET_FLAG_DIMMED
    );
    m_hoverCellX = m_hoverCellY = 0;
    m_cursorActive = false;
    gHeroMoving = false;
    if (gCurPlayerData->CurrentTown() != GAME_TOWN_NONE) {
        townPointer = gGame->GetTown(gCurPlayerData->m_currentTown);
        m_mapOriginX = townPointer->m_x - ADVMGR_VIEW_CENTER;
        m_mapOriginY = townPointer->m_y - ADVMGR_VIEW_CENTER;
    } else if (gCurPlayerData->CurrentHero() != HERO_ID_NONE) {
        MobilizeCurrHero(false);
    } else if (gCurPlayerData->m_heroCount > 0) {
        heroPtr = &gGame->m_heroRecs[gCurPlayerData->m_heroIds[0]];
        m_mapOriginX = heroPtr->m_x - ADVMGR_VIEW_CENTER;
        m_mapOriginY = heroPtr->m_y - ADVMGR_VIEW_CENTER;
    } else if (gCurPlayerData->m_townCount > 0) {
        firstTownPointer = &gGame->m_castleRecs[gCurPlayerData->m_townIds[0]];
        m_mapOriginX = firstTownPointer->m_x - ADVMGR_VIEW_CENTER;
        m_mapOriginY = firstTownPointer->m_y - ADVMGR_VIEW_CENTER;
    } else {
        m_mapOriginX = 0;
        m_mapOriginY = 0;
    }
    m_currentTerrain = gGroundToTerrain
        [GetCell(m_mapOriginX + ADVMGR_VIEW_CENTER, m_mapOriginY + ADVMGR_VIEW_CENTER)
             ->m_tileIndex];
    PlayMusic(TERRAIN_MUSIC_TRACK(m_currentTerrain));
    SetEnvironmentOrigin(m_mapOriginX + ADVMGR_VIEW_CENTER, m_mapOriginY + ADVMGR_VIEW_CENTER, 1);
    gMouseManager->MouseCoords(x, y);
    gMouseManager->WarpPointer(x - 20, y - 20);
    Reseed(0, 0);
    CheckDimNextHeroBut();
}

void advManager::LoadRemote(void) {
    gMouseManager->ReallyHidePointer();
    if (gThisNetHumanPlayer[gCurPlayer])
        gMouseManager->SetPointer("advmice.mse", ADVENTURE_POINTER_DEFAULT);
    gGame->LoadGame("REMOTE.GAM", false, true);
    if (gThisNetHumanPlayer[gCurPlayer])
        gGame->CancelComputerScreen();
    gGame->DoNewTurn();
    UpdateHeroLocators(true, 1);
    UpdateTownLocators(true, 1);
    UpdateRadar(true, false);
    gMouseManager->ReallyShowPointer();
    UpdBottomView(true, true, true);
    if ((gGame->m_day != 1 || (gGame->m_week == 1 && gGame->m_month == 1)) && gRemoteOn
        && gThisNetHumanPlayer[gCurPlayer] && gForceSwitchMusic == FORCED_MUSIC_IDLE) {
        PlayMusic(MUSIC_TRACK_NETWORK_TURN);
        gForceSwitchMusic = KBTickCount();
    }
    gAdvManager->ForceNewHover();
}

RemoteMessage* advManager::CheckHandleNet(void) {
    RemoteMessage* receivedPacket;
    i32 exitedFlag;
    RemoteSaveHeader saveHeader;
    RemotePlayerExit playerExit;

    receivedPacket = GetRemoteData(true);
    if (receivedPacket && receivedPacket->type == REMOTE_MESSAGE_RELIABLE) {
        RecordReader payload = RemotePayloadReader(*receivedPacket);
        switch (receivedPacket->command) {
            case BOX_REMOTE_SAVE:
                ReadRemoteSaveHeader(payload, saveHeader);
                exitedFlag = saveHeader.playerExited;
                if (!gGame->ReceiveSaveGame(saveHeader.saveSize, receivedPacket->sender))
                    ShutDown(NULL);
                if (exitedFlag)
                    ReceiveRemotePlayerExit(receivedPacket->sender, 0, true, false);
                LoadRemote();
                break;
            case REMOTE_COMMAND_CHAT:
                PopNetBox(receivedPacket->payload.data);
                break;
            case REMOTE_COMMAND_HERO_TOWN_DATA:
                if (gInCombat)
                    return receivedPacket;
                else
                    DoNetCombat(receivedPacket);
                break;
            case REMOTE_COMMAND_PLAYER_EXIT:
                ReadRemotePlayerExit(payload, playerExit);
                ReceiveRemotePlayerExit(playerExit.position, playerExit.hadControl, false, false);
                break;
            default:
                return receivedPacket;
        }
    }
    return NULL;
}

i16 advManager::CheckHandleNetPlayerWait(struct tag_message& message, b8 doMain) {
    if (message.type == MESSAGE_MOUSE_MOVE)
        gMouseManager->Main(message);

    CheckDoMain(1, doMain);
    if (message.type == MESSAGE_KEY_DOWN) {
        switch (message.keyCode) {
            case INPUT_SCAN_F2:
                PopNetBox(NULL);
                break;

            case INPUT_SCAN_Q:
                if (message.modifiers & MESSAGE_MODIFIER_CONTROL_KEYS) {
                    message.type = MESSAGE_EXECUTIVE;
                    message.executiveCommand = EXECUTIVE_COMMAND_TERMINATE_LOOP;
                    return MESSAGE_DISPATCH_FORWARD;
                }

            default:
                break;
        }
    }

    UpdBottomView(false, true, true);
    return MESSAGE_DISPATCH_CONTINUE;
}

void advManager::TrimLoopingSounds(i32 maxSamples) {
    if (gHighMemBuffer > 0)
        maxSamples += gHighMemBuffer / HIGH_MEMORY_BUFFER_DIVISOR;

    if (maxSamples >= ADVMGR_ENVIRONMENT_SOUND_COUNT)
        return;

    i8 keep[ADVMGR_ENVIRONMENT_SOUND_COUNT];
    i32 loaded = 0;
    memset(keep, 0, sizeof(keep));

    i32 i;
    for (i = 0; i < ADVMGR_ACTIVE_SOUND_COUNT; ++i) {
        if (m_activeSounds[i].soundId >= 0
            && m_activeSounds[i].soundId < ADVMGR_ENVIRONMENT_SOUND_COUNT)
            ++keep[m_activeSounds[i].soundId];
    }

    for (i = 0; i < ADVMGR_ENVIRONMENT_SOUND_COUNT; ++i) {
        if (keep[i] != 0)
            ++loaded;
    }

    if (loaded < maxSamples) {
        for (i = 0; i < ADVMGR_ENVIRONMENT_SOUND_COUNT; ++i) {
            if (keep[i] == 0 && m_loopingSamples[i] != NULL) {
                ++keep[i];
                ++loaded;
                if (loaded >= maxSamples)
                    goto disposeSamples;
            }
        }
    }

disposeSamples:
    for (i = 0; i < ADVMGR_ENVIRONMENT_SOUND_COUNT; ++i) {
        if (m_loopingSamples[i] != NULL && keep[i] == 0) {
            gResourceManager->Dispose(m_loopingSamples[i]);
            m_loopingSamples[i] = NULL;
        }
    }
}

void advManager::DisableButtons(void) {
    if (gAdvManager->m_active != 1)
        return;
    struct tag_message message;
    SET_ADVENTURE_BUTTON_FLAGS(message, m_adventureWindow, WIDGET_COMMAND_CLEAR_FLAGS);
}

void advManager::EnableButtons(void) {
    if (gAdvManager->m_active != 1)
        return;
    struct tag_message message;
    SET_ADVENTURE_BUTTON_FLAGS(message, m_adventureWindow, WIDGET_COMMAND_SET_FLAGS);
}

void advManager::SaveAdventureBorder(void) {
    if (m_adventureBorder != NULL)
        return;

    m_adventureBorder = static_cast<u8*>(malloc(BORDER_BUFFER_SIZE));
    u8* savedPixels = m_adventureBorder;
    u8* screen = gWindowManager->m_screen->m_pixels;
    i32 row;
    for (row = 0; row < BORDER_EDGE_SIZE; ++row) {
        memcpy(savedPixels, screen, ADVENTURE_VIEWPORT_EXTENT);
        screen += LOGICAL_SCREEN_WIDTH;
        savedPixels += ADVENTURE_VIEWPORT_EXTENT;
    }
    for (row = BORDER_EDGE_SIZE; row < BORDER_MIDDLE_END; ++row) {
        memcpy(savedPixels, screen, BORDER_SIDE_BYTES);
        memcpy(savedPixels + BORDER_SIDE_BYTES, screen + BORDER_MIDDLE_END, BORDER_SIDE_BYTES);
        screen += LOGICAL_SCREEN_WIDTH;
        savedPixels += BORDER_SAVED_SIDE_BYTES;
    }
    for (row = BORDER_MIDDLE_END; row < LOGICAL_SCREEN_HEIGHT; ++row) {
        memcpy(savedPixels, screen, ADVENTURE_VIEWPORT_EXTENT);
        screen += LOGICAL_SCREEN_WIDTH;
        savedPixels += ADVENTURE_VIEWPORT_EXTENT;
    }
}

void advManager::DrawAdventureBorder(void) {
    u8* savedPixels;
    u8* screen;
    i32 row;

    if (m_adventureBorder == NULL)
        return;
    if (gNoBorder != false)
        return;

    screen = gWindowManager->m_screen->m_pixels;
    savedPixels = m_adventureBorder;
    for (row = 0; row < BORDER_EDGE_SIZE; ++row) {
        memcpy(screen, savedPixels, ADVENTURE_VIEWPORT_EXTENT);
        screen += LOGICAL_SCREEN_WIDTH;
        savedPixels += ADVENTURE_VIEWPORT_EXTENT;
    }
    for (row = BORDER_EDGE_SIZE; row < BORDER_MIDDLE_END; ++row) {
        memcpy(screen, savedPixels, BORDER_SIDE_BYTES);
        memcpy(screen + BORDER_MIDDLE_END, savedPixels + BORDER_SIDE_BYTES, BORDER_SIDE_BYTES);
        screen += LOGICAL_SCREEN_WIDTH;
        savedPixels += BORDER_SAVED_SIDE_BYTES;
    }
    for (row = BORDER_MIDDLE_END; row < LOGICAL_SCREEN_HEIGHT; ++row) {
        memcpy(screen, savedPixels, ADVENTURE_VIEWPORT_EXTENT);
        screen += LOGICAL_SCREEN_WIDTH;
        savedPixels += ADVENTURE_VIEWPORT_EXTENT;
    }
}

i32 gLimitUpdMinX = UPDATE_NONE;
i32 gCurBottomViewEnemy = BOTTOM_VIEW_NO_ENEMY;
i32 gLastHourGlassPhase = 1;
i32 gUnusedAdventureValue = 28;
class heroWindow* gAdventurePanel;
i32 gFrameStep;
char gArmySizeName[12];
i32 gLimitUpdMaxX;
i32 gLimitUpdMaxY;
b8 gPrefsChanged;
i32 gLimitUpdMinY;
i8 gComboDraw[17][17];
b8 gFreshSave;
i32 gLastAnimFrame;
const i32 gEnvironmentVolume[8] = {127, 96, 63, 31, 21};

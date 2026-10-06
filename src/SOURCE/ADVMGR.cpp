#include <match.h>

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
#include <SOURCE/searchArray.h>
#include <SOURCE/town.h>
#include <SOURCE/wingraph.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The route-overlay byte at (column, row) of this->m_visibilityMap, indexed
// row-major as row * size + column.
#define ADVMGR_VISIBILITY_AT(column, row)                                                          \
    (*(m_visibilityMap + (column) + (row) * MAP_CELL_GRID_SIZE))

// DrawCell's per-call drawing state, kept in module storage.
DATA(0x004a65c8)
i32 s_drawCloudFrame;
DATA(0x004a6714)
u16 s_drawGroundTile;
DATA(0x004a672c)
i8 s_drawFlipCloud;
DATA(0x004a65a4)
u8 s_drawTileset;
DATA(0x004a65d4)
i32 s_drawCovered;
DATA(0x004a65e8)
i32 s_drawStoneTile;

VA(0x00401000, 0x2af)
advManager::advManager(void) {
    i32 i;

    m_groundTiles = NULL;
    m_puzzleIcon = NULL;
    m_mapOriginX = 0;
    m_mapOriginY = 0;
    m_updateMinX = 0;
    m_updateMinY = 0;
    m_updateMaxX = 0;
    m_updateMaxY = 0;
    m_selectedCell = ADVMGR_COMMAND_NONE;
    m_cursorActive = 0;
    m_identifyHeroActive = 0;
    m_drawHeroShadows = 1;
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
    m_puzzleIcon = NULL;
    m_cloudOverlayIcon = NULL;
    m_boatShadowIcon = NULL;
    m_groundTiles = NULL;
    m_cloudTiles = NULL;
    m_stoneTiles = NULL;
    m_adventureWindow = NULL;
    m_visibilityMap = NULL;
    m_heroContextLocked = 0;
    m_townContextLocked = 0;
    bShowIt = 1;
    m_lastQuickViewX = QUICK_VIEW_NONE;
    m_lastQuickViewY = QUICK_VIEW_NONE;
    m_animationPhases[ANIMATION_PHASE_COLUMN_0] = ANIMATION_PHASE_COLUMN_0_INITIAL;
    m_animationPhases[ANIMATION_PHASE_COLUMN_1] = ANIMATION_PHASE_COLUMN_1_INITIAL;
    m_animationPhases[ANIMATION_PHASE_COLUMN_2] = ANIMATION_PHASE_COLUMN_2_INITIAL;
    m_animationPhases[ANIMATION_PHASE_COLUMN_3] = ANIMATION_PHASE_COLUMN_3_INITIAL;
    m_mapData = gpGame->GetWorldMapData();
    gMapX = 0;
    gMapY = 0;
    m_cursorFrameCount = 0;
    m_cursorCycle = 0;
    m_cursorTurning = 0;
}

// InitMainClasses deletes gpAdvManager through this vtable-reset destructor.
VA(0x004012af, 0x14)
advManager::~advManager() {}

VA(0x004012c3, 0xd5d)
H1_ENUM_RETURN(BaseManagerStatus, i16) advManager::Open(i16 id) {
    i32 savedShowIt;
    i32 firstTime;
    i32 oldPlayerVal;
    i32 oldVolume;
    i32 i;

    firstTime = 1;
    gCurBottomView = BOTTOM_VIEW_NONE;
    m_openState = 0;
    bShowIt = 0;
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
    if (gbThisNetHumanPlayer[giCurPlayer])
        gpMouseManager->SetPointer("advmice.mse", ADVENTURE_POINTER_DEFAULT);
    else
        gpMouseManager->SetPointer("advmice.mse", ADVENTURE_POINTER_WAIT);
    if (m_visibilityMap == NULL) {
        m_visibilityMap = new i8[MAP_CELL_GRID_SIZE * MAP_CELL_GRID_SIZE];
        if (m_visibilityMap == NULL)
            MemError();
    }
    m_routeShown = 0;
    gpWindowManager->AddWindow(m_adventureWindow, 0, 1);
    if (m_groundTiles == NULL)
        m_groundTiles = gpResourceManager->GetTileset("ground32.til");
    if (m_cloudTiles == NULL)
        m_cloudTiles = gpResourceManager->GetTileset("clof32.til");
    if (m_stoneTiles == NULL)
        m_stoneTiles = gpResourceManager->GetTileset("ston.til");
    if (m_cloudOverlayIcon == NULL)
        m_cloudOverlayIcon = gpResourceManager->GetIcon("clop32.icn");
    if (m_objectIcons[TILESET_OBJ32_00] == NULL)
        m_objectIcons[TILESET_OBJ32_00] = gpResourceManager->GetIcon("obj32-00.icn");
    if (m_objectIcons[TILESET_OBJ32_01] == NULL)
        m_objectIcons[TILESET_OBJ32_01] = gpResourceManager->GetIcon("obj32-01.icn");
    if (m_objectIcons[TILESET_OBJ32_02] == NULL)
        m_objectIcons[TILESET_OBJ32_02] = gpResourceManager->GetIcon("obj32-02.icn");
    if (m_objectIcons[TILESET_OBJ32_03] == NULL)
        m_objectIcons[TILESET_OBJ32_03] = gpResourceManager->GetIcon("obj32-03.icn");
    if (m_objectIcons[TILESET_OBJ32_04] == NULL)
        m_objectIcons[TILESET_OBJ32_04] = gpResourceManager->GetIcon("obj32-04.icn");
    if (m_objectIcons[TILESET_OBJ32_05] == NULL)
        m_objectIcons[TILESET_OBJ32_05] = gpResourceManager->GetIcon("obj32-05.icn");
    if (m_objectIcons[TILESET_OBJ32_06] == NULL)
        m_objectIcons[TILESET_OBJ32_06] = gpResourceManager->GetIcon("obj32-06.icn");
    if (m_objectIcons[TILESET_OBJ32_07] == NULL)
        m_objectIcons[TILESET_OBJ32_07] = gpResourceManager->GetIcon("obj32-07.icn");
    if (m_objectIcons[TILESET_MTN32] == NULL)
        m_objectIcons[TILESET_MTN32] = gpResourceManager->GetIcon("mtn32.icn");
    if (m_objectIcons[TILESET_TREE32] == NULL)
        m_objectIcons[TILESET_TREE32] = gpResourceManager->GetIcon("tree32.icn");
    if (m_objectIcons[TILESET_TOWN32] == NULL)
        m_objectIcons[TILESET_TOWN32] = gpResourceManager->GetIcon("town32.icn");
    if (m_objectIcons[TILESET_RSRC32] == NULL)
        m_objectIcons[TILESET_RSRC32] = gpResourceManager->GetIcon("rsrc32.icn");
    if (m_objectIcons[TILESET_MONS32] == NULL)
        m_objectIcons[TILESET_MONS32] = gpResourceManager->GetIcon("mons32.icn");
    if (m_objectIcons[TILESET_ART32] == NULL)
        m_objectIcons[TILESET_ART32] = gpResourceManager->GetIcon("art32.icn");
    if (m_objectIcons[TILESET_FLAG32] == NULL)
        m_objectIcons[TILESET_FLAG32] = gpResourceManager->GetIcon("flag32.icn");
    if (m_objectIcons[TILESET_RESSMALL] == NULL)
        m_objectIcons[TILESET_RESSMALL] = gpResourceManager->GetIcon("ressmall.icn");
    if (m_objectIcons[TILESET_HOURGLAS] == NULL)
        m_objectIcons[TILESET_HOURGLAS] = gpResourceManager->GetIcon("hourglas.icn");
    if (m_objectIcons[TILESET_ROUTE] == NULL)
        m_objectIcons[TILESET_ROUTE] = gpResourceManager->GetIcon("route.icn");
    if (m_objectIcons[TILESET_SMCREST] == NULL)
        m_objectIcons[TILESET_SMCREST] = gpResourceManager->GetIcon("smcrest.icn");
    if (m_objectIcons[TILESET_STONBACK] == NULL)
        m_objectIcons[TILESET_STONBACK] = gpResourceManager->GetIcon("stonback.icn");
    if (m_objectIcons[TILESET_MINIMON] == NULL)
        m_objectIcons[TILESET_MINIMON] = gpResourceManager->GetIcon("minimon.icn");
    if (m_heroIcons[ADVMGR_HERO_ICON_KNIGHT] == NULL)
        m_heroIcons[ADVMGR_HERO_ICON_KNIGHT] = gpResourceManager->GetIcon("kngt32.icn");
    if (m_heroIcons[ADVMGR_HERO_ICON_BARBARIAN] == NULL)
        m_heroIcons[ADVMGR_HERO_ICON_BARBARIAN] = gpResourceManager->GetIcon("barb32.icn");
    if (m_heroIcons[ADVMGR_HERO_ICON_SORCERESS] == NULL)
        m_heroIcons[ADVMGR_HERO_ICON_SORCERESS] = gpResourceManager->GetIcon("sorc32.icn");
    if (m_heroIcons[ADVMGR_HERO_ICON_WARLOCK] == NULL)
        m_heroIcons[ADVMGR_HERO_ICON_WARLOCK] = gpResourceManager->GetIcon("wrlk32.icn");
    if (m_heroIcons[ADVMGR_HERO_ICON_BOAT] == NULL)
        m_heroIcons[ADVMGR_HERO_ICON_BOAT] = gpResourceManager->GetIcon("boat32.icn");
    gLoadingMonoIcon = 1;
    if (m_boatShadowIcon == NULL)
        m_boatShadowIcon = gpResourceManager->GetIcon("shadow32.icn");
    gLoadingMonoIcon = 0;
    if (m_flagIcons[PLAYER_COLOR_BLUE] == NULL)
        m_flagIcons[PLAYER_COLOR_BLUE] = gpResourceManager->GetIcon("b-flag32.icn");
    if (m_flagIcons[PLAYER_COLOR_GREEN] == NULL)
        m_flagIcons[PLAYER_COLOR_GREEN] = gpResourceManager->GetIcon("g-flag32.icn");
    if (m_flagIcons[PLAYER_COLOR_RED] == NULL)
        m_flagIcons[PLAYER_COLOR_RED] = gpResourceManager->GetIcon("r-flag32.icn");
    if (m_flagIcons[PLAYER_COLOR_YELLOW] == NULL)
        m_flagIcons[PLAYER_COLOR_YELLOW] = gpResourceManager->GetIcon("y-flag32.icn");
    if (m_boatFlagIcons[PLAYER_COLOR_BLUE] == NULL)
        m_boatFlagIcons[PLAYER_COLOR_BLUE] = gpResourceManager->GetIcon("b-bflg32.icn");
    if (m_boatFlagIcons[PLAYER_COLOR_GREEN] == NULL)
        m_boatFlagIcons[PLAYER_COLOR_GREEN] = gpResourceManager->GetIcon("g-bflg32.icn");
    if (m_boatFlagIcons[PLAYER_COLOR_RED] == NULL)
        m_boatFlagIcons[PLAYER_COLOR_RED] = gpResourceManager->GetIcon("r-bflg32.icn");
    if (m_boatFlagIcons[PLAYER_COLOR_YELLOW] == NULL)
        m_boatFlagIcons[PLAYER_COLOR_YELLOW] = gpResourceManager->GetIcon("y-bflg32.icn");
    gLoadingMonoIcon = 1;
    if (m_puzzleIcon == NULL)
        m_puzzleIcon = gpResourceManager->GetIcon("radar.icn");
    gLoadingMonoIcon = 0;
    for (i = 0; i < ADVMGR_ENVIRONMENT_SOUND_COUNT; i++)
        m_loopingSamples[i] = NULL;
    for (i = 0; i < ADVMGR_ACTIVE_SOUND_COUNT; i++) {
        m_activeSounds[i].soundId = MAP_SOUND_NONE;
        m_activeSounds[i].volume = ENVIRONMENT_SOUND_DEFAULT_VOLUME;
        m_activeSoundMask = 0;
    }
    GetCursorSampleSet(gConfig.walkSpeed);
    if (!gbThisNetHumanPlayer[giCurPlayer]) {
        gpGame->TurnOnAIMusic();
        SetNoDialogMenus(0);
    } else {
        SetNoDialogMenus(1);
    }
    glTimers[ADVENTURE_FRAME_TIMER_SLOT] = KBTickCount() + TIMER_DELAY;
    // 0x100 has no known producer (no MessageType member); retail keeps it.
    m_messageTypeMask = MESSAGE_KEY_DOWN | MESSAGE_KEY_UP | MESSAGE_MOUSE_MOVE
                        | MESSAGE_LEFT_BUTTON_DOWN | MESSAGE_RIGHT_BUTTON_DOWN | 0x100
                        | MESSAGE_WIDGET;
    gpMouseManager->NewUpdate(1);
    oldVolume = gConfig.soundVolume;
    if (gConfig.soundVolume != SOUND_VOLUME_OFF)
        gConfig.soundVolume = SOUND_VOLUME_LAST;
    SetInitialMapOrigin();
    bShowIt = gbThisNetHumanPlayer[giCurPlayer];
    gpMouseManager->SetColorMice(0);
    oldPlayerVal = giCurPlayer;
    savedShowIt = bShowIt;
    giCurPlayer = giCurWatchPlayer;
    gpCurPlayer = &gpGame->m_players[giCurPlayer];
    bShowIt = 1;
    RedrawAdvScreen(1);
    giCurPlayer = oldPlayerVal;
    bShowIt = savedShowIt;
    gpCurPlayer = &gpGame->m_players[giCurPlayer];
    if (!gbThisNetHumanPlayer[giCurPlayer])
        gpGame->ShowComputerScreen();
    gpMouseManager->ReallyShowPointer();
    KBChangeMenu(hmnuAdv);
    gpWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_STEPS_SHORT, gPalette);
    giBottomViewOverride = BOTTOM_VIEW_NONE;
    gConfig.soundVolume = oldVolume;
    SetVolumes(gConfig.soundVolume, gConfig.musicVolume);
    m_messageMask = BASE_MANAGER_ACCEPT_ADVENTURE;
    m_priority = id;
    m_active = 1;
    strcpy(m_name, "advManager");
    return 0;
}

VA(0x00402020, 0x388)
void advManager::Close(void) {
    i16 index;

    ClearBottomView();
    gpMouseManager->SetPointer(MOUSE_INVALID_CURSOR_FRAME);
    StopAllAudio();
    if (m_adventureBorder) {
        free(m_adventureBorder);
        m_adventureBorder = NULL;
    }
    if (gAdvDisposeLevel <= 1) {
        for (index = 0; index < ADVMGR_OBJECT_ICON_COUNT; index++) {
            if (m_objectIcons[index])
                gpResourceManager->Dispose(m_objectIcons[index]);
            m_objectIcons[index] = NULL;
        }
    }
    if (gAdvDisposeLevel <= 0) {
        gpResourceManager->Dispose(m_puzzleIcon);
        m_puzzleIcon = NULL;
        gpResourceManager->Dispose(m_cloudOverlayIcon);
        m_cloudOverlayIcon = NULL;
        for (index = 0; index < ADVMGR_HERO_ICON_COUNT; index++) {
            gpResourceManager->Dispose(m_heroIcons[index]);
            m_heroIcons[index] = NULL;
        }
        gpResourceManager->Dispose(m_boatShadowIcon);
        m_boatShadowIcon = NULL;
        for (index = 0; index < ADVMGR_PLAYER_COLOR_COUNT; index++) {
            gpResourceManager->Dispose(m_flagIcons[index]);
            m_flagIcons[index] = NULL;
            gpResourceManager->Dispose(m_boatFlagIcons[index]);
            m_boatFlagIcons[index] = NULL;
        }
        gpResourceManager->Dispose(m_groundTiles);
        m_groundTiles = NULL;
        gpResourceManager->Dispose(m_cloudTiles);
        m_cloudTiles = NULL;
        gpResourceManager->Dispose(m_stoneTiles);
        m_stoneTiles = NULL;
    }
    for (index = 0; index < ADVMGR_ENVIRONMENT_SOUND_COUNT; index++) {
        if (m_loopingSamples[index])
            gpResourceManager->Dispose(m_loopingSamples[index]);
        m_loopingSamples[index] = NULL;
    }
    for (index = 0; index < ADVMGR_CURSOR_SAMPLE_COUNT; index++) {
        if (m_cursorSamples[index])
            gpResourceManager->Dispose(m_cursorSamples[index]);
        m_cursorSamples[index] = NULL;
    }
    gpWindowManager->RemoveWindow(m_adventureWindow);
    delete m_adventureWindow;
    m_adventureWindow = NULL;
    if (m_visibilityMap)
        delete m_visibilityMap;
    m_visibilityMap = NULL;
    gCurBottomView = BOTTOM_VIEW_NONE;
    m_active = 0;
}

VA(0x004023a8, 0xa2)
void advManager::GetCursorSampleSet(i32 sampleSet) {
    if (sampleSet >= 1)
        sampleSet = CURSOR_SAMPLE_FAST_SET;
    i8 suffixSample[ADVMGR_CURSOR_SAMPLE_COUNT] = {0, 3, 5, 3, 4, 5, 6};
    for (i32 index = 0; index < ADVMGR_CURSOR_SAMPLE_COUNT; ++index) {
        sprintf(gText, "wsnd%1d%1d.82M", sampleSet, suffixSample[index]);
        m_cursorSamples[index] = gpResourceManager->GetSample(gText);
        m_cursorSamples[index]->m_playbackData.volume = SAMPLE_VOLUME_FULL;
    }
}

VA(0x0040244a, 0x541)
class mapCell* advManager::DoAdvCommand(void) {
    i8 moveDone;
    town* viewTown;
    i8 userStopRequested;
    hero* oldHero;
    i32 oldMapValid;
    tag_message evt;
    i8 hover;
    mapCell* cellPtr;
    i32 anyMoveChanged;
    i16 pos;

    cellPtr = NULL;
    oldHero = gpGame->GetHero(gpCurPlayer->m_currentHero);
    userStopRequested = 0;
    hover = 0;
    switch (m_selectedCell) {
        case ADVMGR_COMMAND_MOVE_TO:
            oldHero->m_destinationX = m_commandTargetX;
            oldHero->m_destinationY = m_commandTargetY;
            goto continue_route;
        case ADVMGR_COMMAND_CONTINUE_ROUTE:
        continue_route:
            gpSearchArray->BuildPath(
                oldHero->m_x,
                oldHero->m_y,
                oldHero->m_destinationX,
                oldHero->m_destinationY,
                SEARCH_UNLIMITED_COST
            );
            if (gpSearchArray->m_pathLength > 0) {
                oldMapValid = m_routeShown;
                MobilizeCurrHero(1);
                if (gConfig.showRoute || oldMapValid)
                    ShowRoute(0, 0, 0);
                else if (m_routeShown && m_selectedCell != ADVMGR_COMMAND_CONTINUE_ROUTE)
                    HideRoute(1, 0, 1);
                gpMouseManager->ReallyHidePointer();
                gpInputManager->Flush();
                for (pos = gpSearchArray->m_pathLength - 1; pos >= 0; pos--) {
                    cellPtr = MoveHero(
                        gpSearchArray->m_directions[pos],
                        pos == 0,
                        &TrigX,
                        &TrigY,
                        &anyMoveChanged,
                        0,
                        &moveDone
                    );
                    UpdateHeroLocator(LOCATOR_SLOT_CURRENT_HERO, 1, 1);
                    if (cellPtr)
                        break;
                    if (anyMoveChanged || moveDone)
                        goto movement_done;
                    evt = gpInputManager->GetEvent();
                    while (evt.type) {
                        if (evt.type == MESSAGE_KEY_DOWN || evt.type == MESSAGE_LEFT_BUTTON_DOWN
                            || evt.type == MESSAGE_RIGHT_BUTTON_DOWN
                            || evt.type == MESSAGE_WIDGET) {
                            userStopRequested = 1;
                            StopCursor(1);
                            goto movement_done;
                        }
                        Process1WindowsMessage();
                        evt = gpInputManager->GetEvent();
                    }
                }
            movement_done:
                if ((pos <= 0 && oldHero->m_x == oldHero->m_destinationX
                     && oldHero->m_y == oldHero->m_destinationY)
                    || (userStopRequested && !gConfig.showRoute) || cellPtr)
                    HideRoute(1, 1, 1);
                else if (m_selectedCell == ADVMGR_COMMAND_CONTINUE_ROUTE || gConfig.showRoute)
                    ShowRoute(0, 1, 1);
                gpMouseManager->ReallyShowPointer();
                UpdBottomView(1, 1, 1);
                if (cellPtr) {
                    StopCursor(1);
                    DoEvent(cellPtr, TrigX, TrigY);
                    cellPtr = NULL;
                }
                Reseed(0, 0);
                hover = 1;
                CheckDimHero();
            }
            break;
        case ADVMGR_COMMAND_OCCUPIED_TOWN_VIEW:
            DemobilizeCurrHero();
            gpMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
            viewTown = gpGame->GetTown(oldHero->m_occupiedTown);
            viewTown->View();
            cellPtr = NULL;
            break;
        case ADVMGR_COMMAND_TOWN_VIEW:
            DemobilizeCurrHero();
            gpMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
            cellPtr = GetCell(
                gpGame->GetTown(gpCurPlayer->m_currentTown)->m_x,
                gpGame->GetTown(gpCurPlayer->m_currentTown)->m_y
            );
            gpGame->GetTown(gpCurPlayer->m_currentTown)->View();
            cellPtr = NULL;
            break;
        case ADVMGR_COMMAND_HERO_VIEW:
            gpMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
            gpGame->GetHero(gpCurPlayer->m_currentHero)->HeroView(0);
            RedrawAdvScreen(1);
            gpWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_STEPS_SHORT, NULL);
            break;
        case ADVMGR_COMMAND_SELECT_HERO:
            SetHeroContext(
                GetCell(m_mapOriginX + m_lastHoverCell, m_mapOriginY + m_hoverCellY)
                    ->m_objectMetadata,
                0
            );
            break;
        case ADVMGR_COMMAND_SELECT_TOWN:
            SetTownContext(GetCell(m_mapOriginX + m_lastHoverCell, m_mapOriginY + m_hoverCellY)
                               ->m_objectMetadata);
            break;
        case ADVMGR_COMMAND_NONE:
            break;
    }
    m_selectedCell = ADVMGR_COMMAND_NONE;
    m_lastHoverCell = m_hoverCellY = CURSOR_INVALID_POSITION;
    if (hover)
        ForceNewHover();
    return cellPtr;
}

VA(0x0040298b, 0xc7e)
H1_ENUM_RETURN(MessageDispatchResult, i16) advManager::Main(struct tag_message& message) {
    DATA(0x004a6760)
    static i32 gCheatSeq = 0;
    i32 yPos;
    i32 xPos;
    H1_ENUM_LOCAL(MessageDispatchResult, i32) result;
    mapCell* location;
    hero* curHero;
    i32 townIndex;
    i32 amount;
    i32 quit;
    i32 movedSet;
    i8 bEnded;
    i32 newHelpText;
    i32 orient;

    if (glTimers[ADVENTURE_FRAME_TIMER_SLOT] < KBTickCount() && ComboDraw(1))
        UpdateScreen(1, 0);
    if (gGameOver) {
        message.type = MESSAGE_EXECUTIVE;
        message.executiveCommand = EXECUTIVE_COMMAND_TERMINATE_LOOP;
        return MESSAGE_DISPATCH_FORWARD;
    }
    if (!gbHumanPlayer[giCurPlayer] && (!gRemoteOn || giThisGamePos == giHostGamePos)) {
        gpPhilAI->DoAI(giCurPlayer);
        gpGame->NextPlayer();
        return MESSAGE_DISPATCH_CONSUME;
    }
    CheckHandleNet();
    if (!gbThisNetHumanPlayer[giCurPlayer])
        return CheckHandleNetPlayerWait(message, 0);
    if (giScreenScroll && gForegroundApp)
        CheckScreenScroll();
    if (!(message.type & m_messageTypeMask)) {
        if (message.type) {
            message.type = MESSAGE_NONE;
            return MESSAGE_DISPATCH_FORWARD;
        }
        return MESSAGE_DISPATCH_CONTINUE;
    }
    if (!gbNoSound && gConfig.musicVolume && gForceSwitchMusic > 0
        && KBTickCount() - gForceSwitchMusic > FORCED_MUSIC_DELAY
        && GetCurrentTrack() == MUSIC_TRACK_NETWORK_TURN) {
        gForceSwitchMusic = FORCED_MUSIC_IDLE;
        PlayMusic(m_currentTerrain);
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
                        if (newHelpText >= 0)
                            NormalDialog(gAdvMenuHelp[newHelpText], NORMAL_DIALOG_TYPE_QUICK_VIEW);
                        break;
                }
                break;
            case MESSAGE_KEY_DOWN:
                orient = -1;
                if (gpCurPlayer->CurrentHero() != HERO_ID_NONE)
                    curHero = gpGame->GetHero(gpCurPlayer->m_currentHero);
                else
                    curHero = NULL;
                if (giDebugLevel < 1
                    && (message.keyCode == INPUT_SCAN_F3 || message.keyCode == INPUT_SCAN_F4
                        || message.keyCode == INPUT_SCAN_F5 || message.keyCode == INPUT_SCAN_F6
                        || message.keyCode == INPUT_SCAN_F7 || message.keyCode == INPUT_SCAN_F8
                        || message.keyCode == INPUT_SCAN_F9 || message.keyCode == INPUT_SCAN_F10
                        || message.keyCode == INPUT_SCAN_F11 || message.keyCode == INPUT_SCAN_F12))
                    break;
                switch (message.keyCode) {
                    case INPUT_SCAN_F2:
                        PopNetBox(NULL);
                        break;
                    case INPUT_SCAN_F3:
                        gpGame->m_playerDead[1] = 1;
                        gpGame->m_playerDead[2] = 1;
                        gpGame->m_playerDead[3] = 1;
                        CheckEndGame(1);
                        break;
                    case INPUT_SCAN_F5:
                        gpGame->m_playerDead[0] = 1;
                        CheckEndGame(0);
                        break;
                    case INPUT_SCAN_F6:
                        if (curHero) {
                            for (amount = 0; amount < HERO_SPELL_SLOT_COUNT; amount++)
                                curHero->AddSpell(amount, CHEAT_SPELL_CHARGES, 0);
                        }
                        break;
                    case INPUT_SCAN_F7:
                        if (curHero)
                            GiveExperience(curHero, CHEAT_EXPERIENCE_AMOUNT, 1);
                        break;
                    case INPUT_SCAN_F8:
                        if (curHero) {
                            gpGame->GiveArmy(
                                &curHero->m_army,
                                CREATURE_DRAGON,
                                1,
                                ARMY_GROUP_EMPTY_SLOT
                            );
                            gpGame->GiveArmy(
                                &curHero->m_army,
                                CREATURE_TROLL,
                                1,
                                ARMY_GROUP_EMPTY_SLOT
                            );
                        }
                        break;
                    case INPUT_SCAN_F9:
                        for (amount = 0; amount < RESOURCE_COUNT; amount++) {
                            gpCurPlayer->m_resources[amount] +=
                                (amount == RESOURCE_GOLD ? CHEAT_GOLD_AMOUNT
                                                         : CHEAT_RESOURCE_AMOUNT);
                        }
                        break;
                    case INPUT_SCAN_F11:
                        if (curHero)
                            curHero->m_remainingMobility = CHEAT_MOBILITY;
                        break;
                    case INPUT_SCAN_F12:
                        gpGame->SetVisibility(
                            CHEAT_REVEAL_CENTER,
                            CHEAT_REVEAL_CENTER,
                            giCurPlayer,
                            CHEAT_REVEAL_RADIUS
                        );
                        UpdateRadar(1, 0);
                        CompleteDraw(0);
                        UpdateScreen(0, 0);
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
                        gCheatSeq =
                            gCheatSeq * CHEAT_SEQUENCE_RADIX % CHEAT_SEQUENCE_MODULUS + amount;
                        if (gCheatSeq == CHEAT_REVEAL_MAP) {
                            gpGame->SetVisibility(
                                CHEAT_REVEAL_CENTER,
                                CHEAT_REVEAL_CENTER,
                                0,
                                CHEAT_REVEAL_RADIUS
                            );
                            gpGame->SetVisibility(
                                CHEAT_REVEAL_CENTER,
                                CHEAT_REVEAL_CENTER,
                                1,
                                CHEAT_REVEAL_RADIUS
                            );
                            gpGame->SetVisibility(
                                CHEAT_REVEAL_CENTER,
                                CHEAT_REVEAL_CENTER,
                                2,
                                CHEAT_REVEAL_RADIUS
                            );
                            gpGame->SetVisibility(
                                CHEAT_REVEAL_CENTER,
                                CHEAT_REVEAL_CENTER,
                                3,
                                CHEAT_REVEAL_RADIUS
                            );
                            Reseed(0, 0);
                            UpdateRadar(1, 0);
                            CompleteDraw(0);
                            UpdateScreen(0, 0);
                        }
                        break;
                    case INPUT_SCAN_ESCAPE:
                        break;
                    case INPUT_SCAN_NUMPAD_8:
                        if (message.modifiers & MESSAGE_MODIFIER_CONTROL_KEYS)
                            ScreenScroll(MAP_DIRECTION_NORTH, 0);
                        else
                            orient = MAP_DIRECTION_NORTH;
                        break;
                    case INPUT_SCAN_NUMPAD_9:
                        if (message.modifiers & MESSAGE_MODIFIER_CONTROL_KEYS)
                            ScreenScroll(MAP_DIRECTION_NORTH_EAST, 0);
                        else
                            orient = MAP_DIRECTION_NORTH_EAST;
                        break;
                    case INPUT_SCAN_NUMPAD_6:
                        if (message.modifiers & MESSAGE_MODIFIER_CONTROL_KEYS)
                            ScreenScroll(MAP_DIRECTION_EAST, 0);
                        else
                            orient = MAP_DIRECTION_EAST;
                        break;
                    case INPUT_SCAN_NUMPAD_3:
                        if (message.modifiers & MESSAGE_MODIFIER_CONTROL_KEYS)
                            ScreenScroll(MAP_DIRECTION_SOUTH_EAST, 0);
                        else
                            orient = MAP_DIRECTION_SOUTH_EAST;
                        break;
                    case INPUT_SCAN_NUMPAD_2:
                        if (message.modifiers & MESSAGE_MODIFIER_CONTROL_KEYS)
                            ScreenScroll(MAP_DIRECTION_SOUTH, 0);
                        else
                            orient = MAP_DIRECTION_SOUTH;
                        break;
                    case INPUT_SCAN_NUMPAD_1:
                        if (message.modifiers & MESSAGE_MODIFIER_CONTROL_KEYS)
                            ScreenScroll(MAP_DIRECTION_SOUTH_WEST, 0);
                        else
                            orient = MAP_DIRECTION_SOUTH_WEST;
                        break;
                    case INPUT_SCAN_NUMPAD_4:
                        if (message.modifiers & MESSAGE_MODIFIER_CONTROL_KEYS)
                            ScreenScroll(MAP_DIRECTION_WEST, 0);
                        else
                            orient = MAP_DIRECTION_WEST;
                        break;
                    case INPUT_SCAN_NUMPAD_7:
                        if (message.modifiers & MESSAGE_MODIFIER_CONTROL_KEYS)
                            ScreenScroll(MAP_DIRECTION_NORTH_WEST, 0);
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
                        ViewWorld(SPELL_VIEW_ALL, 0, 0);
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
                        if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CANCEL)
                            quit = 0;
                        else
                            gGameCommand = amount;
                        break;
                    case INPUT_SCAN_S:
                        SaveGame();
                        break;
                    case INPUT_SCAN_I:
                        if (gpGame->m_campaignType > 0)
                            gpGame->ShowCampaignInfo(gpGame->m_campaignScenario, 1, 0);
                        else
                            gpGame->ShowScenInfo();
                        break;
                    case INPUT_SCAN_T:
                        if (gpCurPlayer->m_townCount >= 0) {
                            if (gpCurPlayer->CurrentTown() == GAME_TOWN_NONE) {
                                townIndex = gpCurPlayer->m_townIds[0];
                            } else {
                                townIndex = 0;
                                for (amount = 0; amount < gpCurPlayer->m_townCount; amount++) {
                                    if (gpCurPlayer->CurrentTown()
                                        == gpCurPlayer->m_townIds[amount]) {
                                        if (amount == gpCurPlayer->m_townCount - 1)
                                            townIndex = gpCurPlayer->m_townIds[0];
                                        else
                                            townIndex = gpCurPlayer->m_townIds[amount + 1];
                                    }
                                }
                            }
                            SetTownContext(townIndex);
                        }
                        break;
                    case INPUT_SCAN_H:
                        SetHeroContext(gpCurPlayer->NextHero(0), 0);
                        break;
                    case INPUT_SCAN_ENTER:
                        if (gpCurPlayer->CurrentTown() != GAME_TOWN_NONE) {
                            m_selectedCell = ADVMGR_COMMAND_TOWN_VIEW;
                            DoAdvCommand();
                        } else if (gpCurPlayer->CurrentHero() != HERO_ID_NONE) {
                            m_selectedCell = ADVMGR_COMMAND_HERO_VIEW;
                            DoAdvCommand();
                        }
                        break;
                }
                if (gpCurPlayer->m_currentHero != HERO_ID_NONE && orient >= 0) {
                    HideRoute(1, 1, 1);
                    gpMouseManager->ReallyHidePointer();
                    location = MoveHero(orient, 1, &TrigX, &TrigY, &movedSet, 0, &bEnded);
                    UpdateHeroLocator(LOCATOR_SLOT_CURRENT_HERO, 1, 1);
                    if (location) {
                        StopCursor(1);
                        DoEvent(location, TrigX, TrigY);
                        location = NULL;
                    }
                    Reseed(0, 0);
                    ForceNewHover();
                    UpdBottomView(1, 1, 1);
                    CheckDimHero();
                    gpMouseManager->ReallyShowPointer();
                }
                break;
        }
    }
    if (location)
        DoEvent(location, TrigX, TrigY);
    if (gGameOver || quit == 1 || gMenuCommand != APP_MENU_NONE) {
        message.type = MESSAGE_EXECUTIVE;
        message.executiveCommand = EXECUTIVE_COMMAND_TERMINATE_LOOP;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return result;
}

VA(0x00403609, 0x17)
void advManager::Reseed(i32, i32) {
    giSeedingValid = 0;
}

VA(0x00403620, 0xe55)
H1_ENUM_RETURN(MessageDispatchResult, i32) advManager::ProcessSelect(struct tag_message* message, class mapCell** eventCell) {
    i32 iPage;
    i16 mouseX;
    i16 objectTypeState;
    i16 mouseY;
    i16 objectIdIndex;
    i32 visible;
    mapCell* theCell;
    tag_message mouseMsg;
    tag_message inputMessage;
    i8 mobileResult;
    hero* currentHero;

    visible = 1;
    switch (message->id) {
        case ADVENTURE_CONTROL_HERO_LOCATOR_1:
        case ADVENTURE_CONTROL_HERO_LOCATOR_2:
        case ADVENTURE_CONTROL_HERO_LOCATOR_3:
        case ADVENTURE_CONTROL_HERO_LOCATOR_4:
            iPage = (message->id - ADVENTURE_CONTROL_HERO_LOCATOR_1)
                    / (ADVENTURE_CONTROL_HERO_LOCATOR_2 - ADVENTURE_CONTROL_HERO_LOCATOR_1);
            if (iPage >= gpCurPlayer->m_heroCount)
                break;
            objectTypeState = gpCurPlayer->m_heroIds[gpCurPlayer->m_heroLocatorPage + iPage];
            if (message->modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON) {
                HeroQuickView(objectTypeState, iPage, QUICK_VIEW_AT_LOCATOR, QUICK_VIEW_AT_LOCATOR);
            } else if (objectTypeState == gpCurPlayer->CurrentHero()) {
                m_selectedCell = ADVMGR_COMMAND_HERO_VIEW;
                DoAdvCommand();
            } else {
                HideRoute(1, 0, 1);
                SetHeroContext(objectTypeState, 0);
            }
            break;
        case ADVENTURE_CONTROL_TOWN_LOCATOR_1:
        case ADVENTURE_CONTROL_TOWN_LOCATOR_2:
        case ADVENTURE_CONTROL_TOWN_LOCATOR_3:
        case ADVENTURE_CONTROL_TOWN_LOCATOR_4:
            objectTypeState = gpCurPlayer->m_townIds
                                  [gpCurPlayer->m_townLocatorPage + message->id
                                   - ADVENTURE_CONTROL_TOWN_LOCATOR_1];
            if (message->modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON) {
                TownQuickView(
                    objectTypeState,
                    message->id - ADVENTURE_CONTROL_TOWN_LOCATOR_1,
                    QUICK_VIEW_AT_LOCATOR,
                    QUICK_VIEW_AT_LOCATOR
                );
            } else {
                HideRoute(1, 0, 1);
                if (objectTypeState == gpCurPlayer->CurrentTown()) {
                    m_selectedCell = ADVMGR_COMMAND_TOWN_VIEW;
                    *eventCell = DoAdvCommand();
                } else {
                    SetTownContext(objectTypeState);
                }
            }
            break;
        case ADVENTURE_CONTROL_HERO_PAGE_PREVIOUS:
            if (gpCurPlayer->m_heroLocatorPage > 0) {
                gpCurPlayer->m_heroLocatorPage--;
                UpdateHeroLocators(1, 1);
            }
            break;
        case ADVENTURE_CONTROL_HERO_PAGE_NEXT:
            if (gpCurPlayer->m_heroLocatorPage + LOCATOR_VISIBLE_COUNT < gpCurPlayer->m_heroCount) {
                gpCurPlayer->m_heroLocatorPage++;
                UpdateHeroLocators(1, 1);
            }
            break;
        case ADVENTURE_CONTROL_HERO_KNOB:
            DoHeroKnob();
            break;
        case ADVENTURE_CONTROL_HERO_SCROLL:
            gpMouseManager->MouseCoords(mouseX, mouseY);
            mouseY -= LOCATOR_SCROLL_MOUSE_BASE_Y;
            if (gpCurPlayer->m_heroCount > LOCATOR_VISIBLE_COUNT) {
                iPage = mouseY
                        / (LOCATOR_SCROLL_MOUSE_SPAN
                           / (gpCurPlayer->m_heroCount - (LOCATOR_VISIBLE_COUNT - 1)));
                if (iPage > gpCurPlayer->m_heroCount - LOCATOR_VISIBLE_COUNT)
                    iPage = gpCurPlayer->m_heroCount - LOCATOR_VISIBLE_COUNT;
            } else {
                iPage = 0;
            }
            gpCurPlayer->m_heroLocatorPage = iPage;
            UpdateHeroLocators(1, 1);
            break;
        case ADVENTURE_CONTROL_TOWN_KNOB:
            DoTownKnob();
            break;
        case ADVENTURE_CONTROL_TOWN_SCROLL:
            gpMouseManager->MouseCoords(mouseX, mouseY);
            mouseY -= LOCATOR_SCROLL_MOUSE_BASE_Y;
            if (gpCurPlayer->m_townCount > LOCATOR_VISIBLE_COUNT) {
                iPage = mouseY
                        / (LOCATOR_SCROLL_MOUSE_SPAN
                           / (gpCurPlayer->m_townCount - (LOCATOR_VISIBLE_COUNT - 1)));
                if (iPage > gpCurPlayer->m_townCount - LOCATOR_VISIBLE_COUNT)
                    iPage = gpCurPlayer->m_townCount - LOCATOR_VISIBLE_COUNT;
            } else {
                iPage = 0;
            }
            gpCurPlayer->m_townLocatorPage = iPage;
            UpdateTownLocators(1, 1);
            break;
        case ADVENTURE_CONTROL_TOWN_PAGE_PREVIOUS:
            if (gpCurPlayer->m_townLocatorPage > 0) {
                gpCurPlayer->m_townLocatorPage--;
                UpdateTownLocators(1, 1);
            }
            break;
        case ADVENTURE_CONTROL_TOWN_PAGE_NEXT:
            if (gpCurPlayer->m_townLocatorPage + LOCATOR_VISIBLE_COUNT < gpCurPlayer->m_townCount) {
                gpCurPlayer->m_townLocatorPage++;
                UpdateTownLocators(1, 1);
            }
            break;
        case ADVENTURE_CONTROL_MAP_VIEW:
            if (!(gpGame->m_mapExtra[m_mapOriginX + m_lastHoverCell][m_mapOriginY + m_hoverCellY]
                  & giCurPlayerBit))
                visible = 0;
            theCell = GetCell(m_mapOriginX + m_lastHoverCell, m_mapOriginY + m_hoverCellY);
            if (message->modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON) {
                if (!visible) {
                    QuickInfo(m_lastHoverCell, m_hoverCellY);
                } else {
                    if (m_lastHoverCell == ADVMGR_VIEW_CENTER && m_hoverCellY == ADVMGR_VIEW_CENTER
                        && gpCurPlayer->CurrentHero() != HERO_ID_NONE && m_heroContextLocked) {
                        objectTypeState = MAP_OBJECT_TRIGGER(MAP_OBJECT_HERO);
                        objectIdIndex = gpCurPlayer->CurrentHero();
                    } else {
                        objectTypeState = theCell->m_triggerType & MAP_TRIGGER_TYPE_MASK;
                        objectIdIndex = theCell->m_objectMetadata;
                    }
                    switch (MAP_PASSIVE_OBJECT(objectTypeState)) {
                        case MAP_OBJECT_HERO:
                            mouseX = m_lastHoverCell * CELL_PIXELS - HERO_QUICK_VIEW_X_OFFSET;
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
                            mouseX = m_lastHoverCell * CELL_PIXELS - TOWN_QUICK_VIEW_X_OFFSET;
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
                            if (gpGame->m_mapExtra[m_mapOriginX + m_lastHoverCell]
                                                  [m_mapOriginY + m_hoverCellY]
                                & giCurPlayerBit)
                                QuickInfo(m_lastHoverCell, m_hoverCellY);
                            break;
                    }
                }
            } else if (visible) {
                currentHero = NULL;
                mobileResult = 0;
                if (gpCurPlayer->m_currentHero != HERO_ID_NONE) {
                    currentHero = gpGame->GetHero(gpCurPlayer->m_currentHero);
                    mobileResult = gpGame->IsMobile(currentHero->m_id);
                }
                if (currentHero) {
                    if (m_lastHoverCell == ADVMGR_VIEW_CENTER && m_hoverCellY == ADVMGR_VIEW_CENTER
                        && gpCurPlayer->CurrentHero() != HERO_ID_NONE && m_heroContextLocked) {
                        m_selectedCell = ADVMGR_COMMAND_HERO_VIEW;
                        DoAdvCommand();
                    } else if ((!mobileResult
                                || (message->modifiers & MESSAGE_MODIFIER_CONTROL_KEYS)
                                || (gConfig.showRoute
                                    && (currentHero->m_destinationX != m_commandTargetX
                                        || currentHero->m_destinationY != m_commandTargetY)))
                               && gpSearchArray->m_cells[m_commandTargetX][m_commandTargetY]
                                      .visited) {
                        currentHero->m_destinationX = m_commandTargetX;
                        currentHero->m_destinationY = m_commandTargetY;
                        ShowRoute(1, 1, 1);
                    } else {
                        *eventCell = DoAdvCommand();
                    }
                } else {
                    objectTypeState = theCell->m_triggerType & MAP_TRIGGER_TYPE_MASK;
                    objectIdIndex = theCell->m_objectMetadata;
                    if (MAP_PASSIVE_OBJECT(objectTypeState) == MAP_OBJECT_HERO) {
                        if (objectIdIndex == gpCurPlayer->CurrentHero()) {
                            m_selectedCell = ADVMGR_COMMAND_HERO_VIEW;
                            DoAdvCommand();
                        } else if (gpGame->GetHero(objectIdIndex)->m_owner == giCurPlayer) {
                            SetHeroContext(objectIdIndex, 0);
                        }
                    }
                    if (MAP_PASSIVE_OBJECT(objectTypeState) == MAP_OBJECT_TOWN) {
                        if (objectIdIndex == gpCurPlayer->CurrentTown()) {
                            m_selectedCell = ADVMGR_COMMAND_TOWN_VIEW;
                            *eventCell = DoAdvCommand();
                        } else if (gpGame->GetTown(objectIdIndex)->m_owner == giCurPlayer) {
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
            gpMouseManager->MouseCoords(mouseX, mouseY);
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
            UpdateRadar(1, 0);
            CompleteDraw(0);
            UpdateScreen(0, 0);
            inputMessage.type = MESSAGE_NONE;
            while (inputMessage.type != MESSAGE_LEFT_BUTTON_UP) {
                Process1WindowsMessage();
                inputMessage = gpInputManager->GetEvent();
                mouseMsg = inputMessage;
                while (inputMessage.type != MESSAGE_LEFT_BUTTON_UP
                       && inputMessage.type != MESSAGE_NONE) {
                    if (inputMessage.type == MESSAGE_MOUSE_MOVE)
                        mouseMsg = inputMessage;
                    Process1WindowsMessage();
                    inputMessage = gpInputManager->GetEvent();
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
                    gpMouseManager->Main(mouseMsg);
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
                    UpdateRadar(1, 0);
                    CompleteDraw(0);
                    UpdateScreen(0, 0);
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
    return 1;
}

VA(0x00404475, 0x1ac)
H1_ENUM_RETURN(MessageDispatchResult, i32) advManager::ProcessDeSelect(
    struct tag_message* message,
    i32* result,
    class mapCell** eventCell
) {
    switch (message->id) {
        case ADVENTURE_CONTROL_CONTINUE_ROUTE:
            m_selectedCell = ADVMGR_COMMAND_CONTINUE_ROUTE;
            *eventCell = DoAdvCommand();
            break;
        case ADVENTURE_CONTROL_ADVENTURE_OPTIONS:
            AdvPanel();
            break;
        case ADVENTURE_CONTROL_GAME_OPTIONS:
            *result = ControlPanel();
            break;
        case ADVENTURE_CONTROL_END_TURN:
            if (gpCurPlayer->HasMobileHero()) {
                NormalDialog(
                    localization::Tr("adventure.confirm_end_turn"),
                    NORMAL_DIALOG_TYPE_YES_NO
                );
                if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CANCEL)
                    break;
            }
            gpGame->NextPlayer();
            break;
        case ADVENTURE_CONTROL_NEXT_HERO:
            HideRoute(1, 0, 1);
            SetHeroContext(gpCurPlayer->NextHero(1), 0);
            break;
        case ADVENTURE_CONTROL_OVERVIEW:
            gpGame->Overview();
            RedrawAdvScreen(1);
            gpWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_STEPS_SHORT, NULL);
            break;
    }
    if (message->id >= BOTTOM_VIEW_DRAW_FIRST_WIDGET
        && message->id <= BOTTOM_VIEW_DRAW_LAST_WIDGET) {
        if (giBottomViewOverride == BOTTOM_VIEW_KINGDOM)
            giBottomViewOverride = BOTTOM_VIEW_NEW_TURN;
        else if (giBottomViewOverride != BOTTOM_VIEW_NONE)
            giBottomViewOverride = BOTTOM_VIEW_NONE;
        else if (gCurBottomView == BOTTOM_VIEW_KINGDOM)
            giBottomViewOverride = BOTTOM_VIEW_NEW_TURN;
        else
            giBottomViewOverride = BOTTOM_VIEW_KINGDOM;
        giBottomViewOverrideEndTime = KBTickCount() + 3000;
        UpdBottomView(1, 1, 1);
    }
    return 1;
}

VA(0x00404621, 0x428)
i32 advManager::ProcessSearch(i32 x, i32 y) {
    class sample* sample = NULL;
    hero* hero;
    i32 pl;
    mapCell* cellPtr;
    tag_message evt;
    i32 gaveArtifact;

    hero = gpGame->GetHero(gpCurPlayer->m_currentHero);
    if (hero->m_remainingMobility != hero->m_mobility) {
        NormalDialog(localization::Tr("adventure.search.requires_full_day"), NORMAL_DIALOG_TYPE_OK);
        return 1;
    }
    MobilizeCurrHero(0);
    CompleteDraw(0);
    UpdateScreen(0, 0);
    if (x == ADVMGR_SEARCH_VIEW_CENTER) {
        x = m_mapOriginX + ADVMGR_VIEW_CENTER;
        y = m_mapOriginY + ADVMGR_VIEW_CENTER;
    }
    cellPtr = GetCell(x, y);
    if (cellPtr->m_objectIndex != MAP_CELL_NO_FRAME
        || cellPtr->m_overlayIndex != MAP_CELL_NO_FRAME) {
        NormalDialog(localization::Tr("adventure.search.clear_ground"), NORMAL_DIALOG_TYPE_OK);
        return 1;
    }
    if (cellPtr->m_tileIndex < MAP_CELL_TILES_PER_TERRAIN) {
        NormalDialog(localization::Tr("adventure.search.on_land"), NORMAL_DIALOG_TYPE_OK);
        return 1;
    }
    if (gbHumanPlayer[giCurPlayer])
        sample = LoadPlaySample("DIGSOUND.82M");
    if (cellPtr->m_objectIndex == MAP_CELL_NO_FRAME) {
        cellPtr->m_objectTileset = TILESET_OBJ32_07;
        cellPtr->m_objectIndex = DIG_HOLE_FRAME;
        cellPtr->m_flags |= MAP_CELL_OBJECT_SHADOW_ONLY;
    }
    CompleteDraw(0);
    UpdateScreen(0, 0);
    GrabScreen();

    if (gpGame->m_ultimateArtifactX == x && gpGame->m_ultimateArtifactY == y
        && gpGame->m_ultimateArtifactId != ARTIFACT_NONE) {
        gaveArtifact = GiveArtifact(hero, gpGame->m_ultimateArtifactId);
        if (gaveArtifact == GIVE_ARTIFACT_NO_SLOT) {
            NormalDialog(
                localization::Tr("adventure.search.no_artifact_room"),
                NORMAL_DIALOG_TYPE_OK,
                0x61,
                0x28
            );
        } else {
            if (gbHumanPlayer[giCurPlayer]) {
                EventSound(MAP_OBJECT_ULTIMATE_ARTIFACT, 0);
                sprintf(
                    gText,
                    "%s%s",
                    localization::Tr("adventure.search.found_prefix"),
                    gArtifactNames[gpGame->m_ultimateArtifactId]
                );
                if (gpGame->m_campaignType > 0
                    && gpGame->m_campaignScenario == CAMPAIGN_SCENARIO_EYE_OF_GOROS) {
                    sprintf(gText, localization::Tr("adventure.search.eye_of_goros_found"));
                    NormalDialog(gText, NORMAL_DIALOG_TYPE_OK, 0xb1, 0x1c);
                } else {
                    NormalDialog(gText, NORMAL_DIALOG_TYPE_OK, 0xb1, 0x1c);
                    hero->ViewArtifact(gpGame->m_ultimateArtifactId, 0);
                }
                PlayMusic(m_currentTerrain);
            } else if (gpGame->m_campaignType > 0
                       && gpGame->m_campaignScenario == CAMPAIGN_SCENARIO_EYE_OF_GOROS) {
                sprintf(gText, localization::Tr("adventure.search.eye_of_goros_enemy"));
                NormalDialog(gText, NORMAL_DIALOG_TYPE_OK, 0xb1, 0x1c);
            }
            gpGame->m_ultimateArtifactId = ARTIFACT_NONE;
        }
    } else if (gbHumanPlayer[giCurPlayer]) {
        NormalDialog(
            localization::Tr("adventure.search.nothing_here"),
            NORMAL_DIALOG_TYPE_OK,
            0x61,
            0x28
        );
    }
    if (gbHumanPlayer[giCurPlayer])
        WaitSample(sample);
    for (pl = 0; pl < gpGame->m_playerCount; pl++)
        ComputeUALoc(pl);
    hero->m_remainingMobility = 0;
    UpdBottomView(1, 1, 1);
    CheckDimHero();
    Reseed(0, 0);
    CheckEndGame(0);
    return 1;
}

VA(0x00404a49, 0xae7)
H1_ENUM_RETURN(MessageDispatchResult, i32) advManager::ProcessHover(struct tag_message* message) {
    i16 xPos;
    i16 yPos;
    town* pTown;
    hero* prevHero;
    mapCell* location;
    i32 nDays;
    H1_ENUM_LOCAL(MapObjectType, i8) trigType;
    i16 heroPosX;
    i16 heroPosY;
    i32 baseFrame;

    switch (message->id) {
        case ADVENTURE_CONTROL_MAP_VIEW:
            gpMouseManager->MouseCoords(xPos, yPos);
            if (xPos > ADVENTURE_VIEWPORT_EXTENT) {
                gpMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
                return 1;
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
            if (m_lastHoverCell != xPos || m_hoverCellY != yPos) {
                m_selectedCell = ADVMGR_COMMAND_NONE;
                m_lastHoverCell = xPos;
                m_hoverCellY = yPos;
                m_commandTargetX = m_mapOriginX + xPos;
                m_commandTargetY = m_mapOriginY + yPos;
                if (m_commandTargetX < 0 || m_commandTargetY < 0
                    || m_commandTargetX > MAP_CELL_GRID_SIZE - 1
                    || m_commandTargetY > MAP_CELL_GRID_SIZE - 1
                    || !(gpGame->m_mapExtra[m_commandTargetX][m_commandTargetY] & giCurPlayerBit)) {
                    gpMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
                    return 1;
                }
                location = GetCell(m_commandTargetX, m_commandTargetY);
                if (gpCurPlayer->m_currentHero == HERO_ID_NONE) {
                    if (MAP_TRIGGER_OBJECT(location->m_triggerType) == MAP_OBJECT_TOWN
                        && gpGame->GetTown(location->m_objectMetadata)->m_owner == giCurPlayer) {
                        gpMouseManager->SetPointer(ADVENTURE_POINTER_TOWN);
                        m_selectedCell = ADVMGR_COMMAND_TOWN_VIEW;
                        return 1;
                    } else if (MAP_TRIGGER_OBJECT(location->m_triggerType) == MAP_OBJECT_HERO
                               && gpGame->GetHero(location->m_objectMetadata)->m_owner
                                      == giCurPlayer) {
                        gpMouseManager->SetPointer(ADVENTURE_POINTER_HERO);
                        m_selectedCell = ADVMGR_COMMAND_HERO_VIEW;
                        return 1;
                    } else {
                        gpMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
                        return 1;
                    }
                } else {
                    prevHero = gpGame->GetHero(gpCurPlayer->m_currentHero);
                    heroPosX = prevHero->m_x - m_mapOriginX;
                    heroPosY = prevHero->m_y - m_mapOriginY;
                    if (xPos == heroPosX && yPos == heroPosY) {
                        gpMouseManager->SetPointer(ADVENTURE_POINTER_HERO);
                        m_selectedCell = ADVMGR_COMMAND_HERO_VIEW;
                        return 1;
                    }
                    if (location->m_secondaryTrigger & MAP_CELL_SECONDARY_BLOCKED) {
                        if (MAP_TRIGGER_OBJECT(location->m_triggerType) == MAP_OBJECT_TOWN) {
                            pTown = gpGame->GetTown(location->m_objectMetadata);
                            if (pTown->m_owner == giCurPlayer && m_commandTargetY >= 1
                                && m_commandTargetY < MAP_CELL_GRID_SIZE - 1
                                && (MAP_TRIGGER_OBJECT(GetCell(m_commandTargetX, m_commandTargetY - 1)->m_triggerType)
                                        == MAP_OBJECT_TOWN
                                    || MAP_TRIGGER_OBJECT(GetCell(m_commandTargetX, m_commandTargetY - 1)
                                            ->m_secondaryTrigger)
                                           == MAP_OBJECT_TOWN)
                                && (MAP_TRIGGER_OBJECT(GetCell(m_commandTargetX, m_commandTargetY + 1)->m_triggerType)
                                        == MAP_OBJECT_TOWN
                                    || MAP_TRIGGER_OBJECT(GetCell(m_commandTargetX, m_commandTargetY + 1)
                                            ->m_secondaryTrigger)
                                           == MAP_OBJECT_TOWN)) {
                                gpMouseManager->SetPointer(ADVENTURE_POINTER_TOWN);
                                m_selectedCell = ADVMGR_COMMAND_SELECT_TOWN;
                                return 1;
                            }
                        }
                        gpSearchArray->m_pathLength = 0;
                        gpMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
                        return 1;
                    }
                    if (!((m_cursorType == ADVMGR_HERO_ICON_BOAT
                           || location->m_tileIndex >= MAP_CELL_TILES_PER_TERRAIN
                           || location->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_HERO)
                           || location->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_SHIP)
                           || location->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_SHIPWRECK))
                          && (m_cursorType != ADVMGR_HERO_ICON_BOAT
                              || location->m_tileIndex < MAP_CELL_TILES_PER_TERRAIN
                              || location->m_triggerType == MAP_OBJECT_TRIGGER(MAP_OBJECT_COAST)))) {
                        gpSearchArray->m_pathLength = 0;
                        gpMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
                        return 1;
                    }
                    SeedTo(m_commandTargetX, m_commandTargetY);
                    if (gpSearchArray->m_cells[m_commandTargetX][m_commandTargetY].visited) {
                        if (gpSearchArray->m_cells[m_commandTargetX][m_commandTargetY].distance
                            <= prevHero->m_remainingMobility) {
                            nDays = 0;
                        } else {
                            nDays =
                                (gpSearchArray->m_cells[m_commandTargetX][m_commandTargetY].distance
                                 - prevHero->m_remainingMobility)
                                    / prevHero->m_mobility
                                + 1;
                            if (nDays > ADVENTURE_POINTER_DAY_LAST)
                                nDays = ADVENTURE_POINTER_DAY_LAST;
                        }
                        baseFrame = nDays * ADVENTURE_POINTER_DAY_STRIDE;
                        switch (MAP_TRIGGER_OBJECT(location->m_triggerType)) {
                            case MAP_OBJECT_SHIP:
                                if (m_cursorType != ADVMGR_HERO_ICON_BOAT) {
                                    gpMouseManager->SetPointer(baseFrame + ADVENTURE_POINTER_SAIL);
                                    m_selectedCell = ADVMGR_COMMAND_MOVE_TO;
                                } else {
                                    gpMouseManager->SetPointer(baseFrame);
                                }
                                break;
                            case MAP_OBJECT_COAST:
                                if (m_cursorType == ADVMGR_HERO_ICON_BOAT)
                                    gpMouseManager->SetPointer(
                                        baseFrame + ADVENTURE_POINTER_DISEMBARK
                                    );
                                else if (mapExtra[m_commandTargetX][m_commandTargetY]
                                         & MAP_EXTRA_MONSTER_ADJACENT)
                                    gpMouseManager->SetPointer(
                                        baseFrame + ADVENTURE_POINTER_ATTACK
                                    );
                                else
                                    gpMouseManager->SetPointer(baseFrame + ADVENTURE_POINTER_MOVE);
                                m_selectedCell = ADVMGR_COMMAND_MOVE_TO;
                                break;
                            case MAP_OBJECT_MONSTER:
                                gpMouseManager->SetPointer(baseFrame + ADVENTURE_POINTER_ATTACK);
                                m_selectedCell = ADVMGR_COMMAND_MOVE_TO;
                                break;
                            case MAP_OBJECT_HERO:
                                if (gpGame->GetHero(location->m_objectMetadata)->m_owner
                                    != giCurPlayer) {
                                    gpMouseManager->SetPointer(
                                        baseFrame + ADVENTURE_POINTER_ATTACK
                                    );
                                    m_selectedCell = ADVMGR_COMMAND_MOVE_TO;
                                } else {
                                    gpMouseManager->SetPointer(
                                        baseFrame + ADVENTURE_POINTER_SELECT_HERO
                                    );
                                    m_selectedCell = ADVMGR_COMMAND_MOVE_TO;
                                }
                                break;
                            case MAP_OBJECT_TOWN:
                                pTown = gpGame->GetTown(location->m_objectMetadata);
                                if ((location->m_triggerType & MAP_TRIGGER_EVENT)
                                    && pTown->m_owner != giCurPlayer && pTown->HasGarrison()) {
                                    gpMouseManager->SetPointer(
                                        baseFrame + ADVENTURE_POINTER_ATTACK
                                    );
                                    m_selectedCell = ADVMGR_COMMAND_MOVE_TO;
                                    break;
                                }
                                goto defaultHover;
                            default:
                            defaultHover:
                                trigType = MAP_TRIGGER_OBJECT(location->m_triggerType);
                                if ((mapExtra[m_commandTargetX][m_commandTargetY]
                                     & MAP_EXTRA_MONSTER_ADJACENT)
                                    && m_cursorType != ADVMGR_HERO_ICON_BOAT
                                    && trigType != MAP_OBJECT_SKELETON
                                    && trigType != MAP_OBJECT_TREASURE_CHEST
                                    && trigType != MAP_OBJECT_CAMPFIRE
                                    && trigType != MAP_OBJECT_ANCIENT_LAMP
                                    && trigType != MAP_OBJECT_RESOURCE
                                    && trigType != MAP_OBJECT_ARTIFACT) {
                                    gpMouseManager->SetPointer(
                                        baseFrame + ADVENTURE_POINTER_ATTACK
                                    );
                                } else if (location->m_triggerType & MAP_TRIGGER_EVENT) {
                                    if (m_cursorType != ADVMGR_HERO_ICON_BOAT) {
                                        switch (MAP_TRIGGER_OBJECT(location->m_triggerType)) {
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
                                                gpMouseManager->SetPointer(
                                                    baseFrame + ADVENTURE_POINTER_ACTION
                                                );
                                                break;
                                            default:
                                                if (mapExtra[m_commandTargetX][m_commandTargetY]
                                                    & MAP_EXTRA_MONSTER_ADJACENT)
                                                    gpMouseManager->SetPointer(
                                                        baseFrame + ADVENTURE_POINTER_ATTACK
                                                    );
                                                else
                                                    gpMouseManager->SetPointer(
                                                        baseFrame + ADVENTURE_POINTER_MOVE
                                                    );
                                                break;
                                        }
                                    } else {
                                        switch (MAP_TRIGGER_OBJECT(location->m_triggerType)) {
                                            case MAP_OBJECT_BUOY:
                                            case MAP_OBJECT_WHIRLPOOL:
                                                gpMouseManager->SetPointer(
                                                    nDays + ADVENTURE_POINTER_WATER_ACTION
                                                );
                                                break;
                                            default:
                                                gpMouseManager->SetPointer(
                                                    baseFrame + ADVENTURE_POINTER_SAIL
                                                );
                                                break;
                                        }
                                    }
                                } else if (m_cursorType == ADVMGR_HERO_ICON_BOAT) {
                                    gpMouseManager->SetPointer(baseFrame + ADVENTURE_POINTER_SAIL);
                                } else {
                                    gpMouseManager->SetPointer(baseFrame + ADVENTURE_POINTER_MOVE);
                                }
                                m_selectedCell = ADVMGR_COMMAND_MOVE_TO;
                                break;
                        }
                        return 1;
                    } else {
                        gpMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
                        return 1;
                    }
                }
            }
            break;
        default:
            if (!(gpMouseManager->m_cursorFrame >= HOVER_SCROLL_FRAME_FIRST
                  && gpMouseManager->m_cursorFrame < HOVER_SCROLL_FRAME_END && MouseInScrollZone()))
                gpMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
            return 1;
    }
    return 1;
}

VA(0x00405530, 0x279)
void advManager::UpdateScreen(i8 cursorUpdate, i8 forceUpdate) {
    if (!forceUpdate && !bShowIt) {
        if (glTimers[ADVENTURE_FRAME_TIMER_SLOT] < KBTickCount())
            glTimers[ADVENTURE_FRAME_TIMER_SLOT] = KBTickCount() + TIMER_DELAY;
        return;
    }
    gpMouseManager
        ->SaveAndDraw(gpWindowManager->m_screen, m_updateMinX, m_updateMinY, cursorUpdate);
    PollSound();
    gScrollX = m_updateMinX;
    gScrollY = m_updateMinY;
    if (gLimitUpdMinX == UPDATE_NONE)
        BlitBitmapToScreen(
            gpWindowManager->m_screen,
            BORDER_EDGE_SIZE,
            BORDER_EDGE_SIZE,
            ADVENTURE_VIEWPORT_INNER_SIZE,
            ADVENTURE_VIEWPORT_INNER_SIZE,
            BORDER_EDGE_SIZE,
            BORDER_EDGE_SIZE
        );
    else
        BlitBitmapToScreen(
            gpWindowManager->m_screen,
            gLimitUpdMinX,
            giLimitUpdMinY,
            giLimitUpdMaxX - gLimitUpdMinX,
            giLimitUpdMaxY - giLimitUpdMinY,
            gLimitUpdMinX,
            giLimitUpdMinY
        );
    gScrollY = 0;
    gScrollX = gScrollY;
    PollSound();
    if (glTimers[ADVENTURE_FRAME_TIMER_SLOT] < KBTickCount()) {
        ++m_updateMaxX;
        if (m_updateMaxX >= UPDATE_FRAME_CYCLE)
            m_updateMaxX = 0;
        glTimers[ADVENTURE_FRAME_TIMER_SLOT] = KBTickCount() + TIMER_DELAY;
        if (m_updateMaxX == UPDATE_FRAME_STEP_1 || m_updateMaxX == UPDATE_FRAME_STEP_3
            || m_updateMaxX == UPDATE_FRAME_STEP_5) {
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
    gpMouseManager->RestoreUnderlying();
    Process1WindowsMessage();
}

VA(0x004057a9, 0x325)
void advManager::CompleteDraw(i16 originX, i16 originY, i32 forceDraw) {
    i32 drawX;
    i32 drawY;

    PollSound();
    if (!forceDraw && !bShowIt)
        return;

    gLimitUpdMinX = UPDATE_NONE;
    m_previousOriginX = m_mapOriginX;
    m_previousOriginY = m_mapOriginY;
    if (gAllBlack)
        m_mapOriginX = m_mapOriginY = 0;
    m_comboHeroDrawn = 0;
    m_forceCompleteDraw = 0;

    for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++)
        DrawCell(
            originX + drawX,
            originY,
            drawX,
            0,
            ADVMGR_DRAW_GROUND | ADVMGR_DRAW_OBJECT,
            0,
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
                0,
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
                    0,
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
                    0,
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
                0,
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
            0,
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
                0,
                forceDraw
            );

    PollSound();
    UpdBottomView(0, 1, 1);
    DrawAdventureBorder();
    if (gAllBlack) {
        m_mapOriginX = m_previousOriginX;
        m_mapOriginY = m_previousOriginY;
    }
}

// CompleteDraw(update) forwards the current map origin.
VA(0x00405ace, 0x2f)
void advManager::CompleteDraw(i32 update) {
    CompleteDraw(m_mapOriginX, m_mapOriginY, update);
}

// Cloud lookup over the x-major visibility bytes: edge masks first, then
// each unseen neighbour, indexed into the cloud table.
VA(0x00405afd, 0x3fe)
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
        if ((gpGame->m_mapExtra[x][y - 1] & giCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_NORTH;
        if ((gpGame->m_mapExtra[x + 1][y] & giCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_EAST;
        if ((gpGame->m_mapExtra[x][y + 1] & giCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_SOUTH;
        if ((gpGame->m_mapExtra[x - 1][y] & giCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_WEST;
        if ((gpGame->m_mapExtra[x + 1][y - 1] & giCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_NORTH_EAST;
        if ((gpGame->m_mapExtra[x + 1][y + 1] & giCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_SOUTH_EAST;
        if ((gpGame->m_mapExtra[x - 1][y + 1] & giCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_SOUTH_WEST;
        if ((gpGame->m_mapExtra[x - 1][y - 1] & giCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_NORTH_WEST;
    } else {
        if ((cloudMask & CLOUD_NORTH) == 0
            && (gpGame->m_mapExtra[x][y - 1] & giCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_NORTH;
        if ((cloudMask & CLOUD_EAST) == 0
            && (gpGame->m_mapExtra[x + 1][y] & giCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_EAST;
        if ((cloudMask & CLOUD_SOUTH) == 0
            && (gpGame->m_mapExtra[x][y + 1] & giCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_SOUTH;
        if ((cloudMask & CLOUD_WEST) == 0
            && (gpGame->m_mapExtra[x - 1][y] & giCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_WEST;
        if ((cloudMask & CLOUD_NORTH_EAST) == 0
            && (gpGame->m_mapExtra[x + 1][y - 1] & giCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_NORTH_EAST;
        if ((cloudMask & CLOUD_SOUTH_EAST) == 0
            && (gpGame->m_mapExtra[x + 1][y + 1] & giCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_SOUTH_EAST;
        if ((cloudMask & CLOUD_SOUTH_WEST) == 0
            && (gpGame->m_mapExtra[x - 1][y + 1] & giCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_SOUTH_WEST;
        if ((cloudMask & CLOUD_NORTH_WEST) == 0
            && (gpGame->m_mapExtra[x - 1][y - 1] & giCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_NORTH_WEST;
    }
    return gCloudType[cloudMask];
}

VA(0x00405efb, 0xe2b)
void advManager::DrawCell(
    i16 mapX,
    i16 mapY,
    i16 screenX,
    i16 screenY,
    H1_ENUM_PARAM(AdventureDrawMask, i8) drawMask,
    i8 drawingPuzzle,
    i8 forceDraw
) {
    i8 savedFrame;
    i32 heroYOffset6;
    i32 savedSuppressed;
    i16 pixelY3;
    i16 pixelX7;
    mapCell* newCell0;
    hero* savedShowHero;
    i8 position;
    i8 flagColorValue;
    i8 drawHeroIcon0;

    if (!forceDraw && !bShowIt)
        return;
    pixelX7 = screenX << CELL_PIXEL_SHIFT;
    pixelY3 = screenY << CELL_PIXEL_SHIFT;
    newCell0 = GetCell(mapX, mapY);
    if (!gAllBlack
        && (mapX < 0 || mapY < 0 || mapX >= MAP_CELL_GRID_SIZE || mapY >= MAP_CELL_GRID_SIZE)) {
        s_drawStoneTile = STONE_TILE_NONE;
        if (mapX == -1) {
            if (mapY == -1)
                s_drawStoneTile = STONE_TILE_TOP_LEFT;
            else if (mapY == MAP_CELL_GRID_SIZE)
                s_drawStoneTile = STONE_TILE_BOTTOM_LEFT;
            else if (mapY >= 0 && mapY < MAP_CELL_GRID_SIZE)
                s_drawStoneTile = ((mapY + STONE_PATTERN_COORDINATE_SHIFT) & CLOUD_VARIANT_MASK)
                                  + STONE_TILE_LEFT_BASE;
        } else if (mapX == MAP_CELL_GRID_SIZE) {
            if (mapY == -1)
                s_drawStoneTile = STONE_TILE_TOP_RIGHT;
            else if (mapY == MAP_CELL_GRID_SIZE)
                s_drawStoneTile = STONE_TILE_BOTTOM_RIGHT;
            else if (mapY >= 0 && mapY < MAP_CELL_GRID_SIZE)
                s_drawStoneTile = ((mapY + STONE_PATTERN_COORDINATE_SHIFT) & CLOUD_VARIANT_MASK)
                                  + STONE_TILE_RIGHT_BASE;
        } else if (mapY == -1) {
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
        TileToBitmap(m_stoneTiles, s_drawStoneTile, gpWindowManager->m_screen, pixelX7, pixelY3);
        return;
    } else {
        if (!((!gAllBlack && (gpGame->m_mapExtra[mapX][mapY] & giCurWatchPlayerBit))
              || drawingPuzzle)) {
            s_drawCovered = 1;
            if (gAllBlack)
                s_drawCloudFrame = 0;
            else
                s_drawCloudFrame = GetCloudLookup(mapX, mapY);
            if (s_drawCloudFrame == 0) {
                if (drawMask & ADVMGR_DRAW_CLOUD)
                    TileToBitmap(
                        m_cloudTiles,
                        (mapX + mapY) & CLOUD_VARIANT_MASK,
                        gpWindowManager->m_screen,
                        pixelX7,
                        pixelY3
                    );
                return;
            }
            if (s_drawCloudFrame >= CLOUD_FLIPPED_FRAME_BASE) {
                s_drawFlipCloud = 1;
                s_drawCloudFrame -= CLOUD_FLIPPED_FRAME_BASE;
            } else {
                s_drawFlipCloud = 0;
            }
            if ((s_drawCloudFrame == CLOUD_X_ALTERNATE_FRAME_1
                 || s_drawCloudFrame == CLOUD_X_ALTERNATE_FRAME_2)
                && (mapX & 1))
                s_drawCloudFrame++;
            if (s_drawCloudFrame == CLOUD_Y_ALTERNATE_FRAME && (mapY & 1))
                s_drawCloudFrame++;
        } else {
            s_drawCovered = 0;
        }
    }
    if (drawMask & ADVMGR_DRAW_CLOUD) {
        if (s_drawCovered) {
            if (s_drawFlipCloud)
                FlipIconToBitmap(
                    m_cloudOverlayIcon,
                    gpWindowManager->m_screen,
                    pixelX7 + CELL_LAST_PIXEL,
                    pixelY3,
                    s_drawCloudFrame - 1,
                    ICON_DRAW_OFFSET_FULL
                );
            else
                IconToBitmap(
                    m_cloudOverlayIcon,
                    gpWindowManager->m_screen,
                    pixelX7,
                    pixelY3,
                    s_drawCloudFrame - 1,
                    ICON_DRAW_OFFSET_FULL
                );
        } else if (m_routeShown && ADVMGR_VISIBILITY_AT(mapX, mapY)) {
            if (ADVMGR_VISIBILITY_AT(mapX, mapY) & ROUTE_CELL_FLIPPED)
                FlipIconToBitmap(
                    m_objectIcons[TILESET_ROUTE],
                    gpWindowManager->m_screen,
                    pixelX7 + CELL_LAST_PIXEL,
                    pixelY3 + ROUTE_DRAW_Y_OFFSET,
                    (ADVMGR_VISIBILITY_AT(mapX, mapY) & ROUTE_CELL_FRAME_MASK) - 1,
                    ICON_DRAW_OFFSET_FULL
                );
            else
                IconToBitmap(
                    m_objectIcons[TILESET_ROUTE],
                    gpWindowManager->m_screen,
                    pixelX7,
                    pixelY3 + ROUTE_DRAW_Y_OFFSET,
                    (ADVMGR_VISIBILITY_AT(mapX, mapY) & ROUTE_CELL_FRAME_MASK) - 1,
                    ICON_DRAW_OFFSET_FULL
                );
        }
        return;
    }
    if (drawMask & ADVMGR_DRAW_GROUND) {
        s_drawGroundTile = newCell0->m_flags;
        s_drawGroundTile <<= MAP_CELL_GROUND_FLIP_SHIFT;
        s_drawGroundTile |= newCell0->m_tileIndex & 0xff;
        TileToBitmap(m_groundTiles, s_drawGroundTile, gpWindowManager->m_screen, pixelX7, pixelY3);
        if (newCell0->m_flags & MAP_CELL_OBJECT_SHADOW_ONLY) {
            s_drawTileset = newCell0->m_objectTileset & MAP_CELL_TILESET_MASK;
            if (!drawingPuzzle || s_drawTileset != TILESET_OBJ32_07
                || newCell0->m_objectIndex != DIG_HOLE_FRAME)
                IconToBitmap(
                    m_objectIcons[s_drawTileset],
                    gpWindowManager->m_screen,
                    pixelX7,
                    pixelY3,
                    newCell0->m_objectIndex,
                    ICON_DRAW_OFFSET_FULL
                );
        }
    }
    if (drawMask & ADVMGR_DRAW_OBJECT) {
        if (!(newCell0->m_flags & MAP_CELL_OBJECT_SHADOW_ONLY)
            && newCell0->m_objectIndex != MAP_CELL_NO_FRAME) {
            s_drawTileset = newCell0->m_objectTileset & MAP_CELL_TILESET_MASK;
            if (s_drawTileset != TILESET_MONS32) {
                IconToBitmap(
                    m_objectIcons[s_drawTileset],
                    gpWindowManager->m_screen,
                    pixelX7,
                    pixelY3,
                    newCell0->m_objectIndex,
                    ICON_DRAW_OFFSET_FULL
                );
                if (newCell0->m_flags & MAP_CELL_OBJECT_ANIMATED)
                    IconToBitmap(
                        m_objectIcons[s_drawTileset],
                        gpWindowManager->m_screen,
                        pixelX7,
                        pixelY3,
                        newCell0->m_objectIndex + m_updateMaxX + 1,
                        ICON_DRAW_OFFSET_FULL
                    );
            }
        }
        if (newCell0->m_flags & MAP_CELL_OBJECT_EXTRA)
            IconToBitmap(
                m_objectIcons[newCell0->m_objectTileset >> MAP_CELL_EXTRA_TILESET_SHIFT],
                gpWindowManager->m_screen,
                pixelX7,
                pixelY3,
                newCell0->m_extraFrame,
                ICON_DRAW_OFFSET_FULL
            );
    }
    if (drawMask & ADVMGR_DRAW_HERO) {
        drawHeroIcon0 = 0;
        savedShowHero = NULL;
        if (!(newCell0->m_flags & MAP_CELL_OBJECT_SHADOW_ONLY)
            && newCell0->m_objectIndex != MAP_CELL_NO_FRAME) {
            s_drawTileset = newCell0->m_objectTileset & MAP_CELL_TILESET_MASK;
            if (s_drawTileset == TILESET_MONS32 && newCell0->m_objectIndex <= CREATURE_COUNT - 1) {
                if (mapX == m_lastQuickViewX && mapY == m_lastQuickViewY) {
                    if (m_mineGuardianFacingLeft)
                        FlipIconToBitmap(
                            m_objectIcons[TILESET_MINIMON],
                            gpWindowManager->m_screen,
                            pixelX7 + 36,
                            pixelY3 - MONSTER_DRAW_Y_OFFSET,
                            newCell0->m_objectIndex * MONSTER_FRAME_STRIDE
                                + MONSTER_FACING_FRAME_BASE,
                            ICON_DRAW_OFFSET_FULL
                        );
                    else
                        IconToBitmap(
                            m_objectIcons[TILESET_MINIMON],
                            gpWindowManager->m_screen,
                            pixelX7,
                            pixelY3 - MONSTER_DRAW_Y_OFFSET,
                            newCell0->m_objectIndex * MONSTER_FRAME_STRIDE
                                + MONSTER_FACING_FRAME_BASE,
                            ICON_DRAW_OFFSET_FULL
                        );
                } else {
                    ClipIconToBitmap(
                        m_objectIcons[TILESET_MINIMON],
                        gpWindowManager->m_screen,
                        pixelX7,
                        pixelY3 - MONSTER_DRAW_Y_OFFSET,
                        newCell0->m_objectIndex * MONSTER_FRAME_STRIDE
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
        if (newCell0->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_SHIP)) {
            flagColorValue = PLAYER_COLOR_NONE;
            position = ADVMGR_HERO_ICON_BOAT;
            savedFrame = GetCursorBaseFrame(gpGame->m_boats[newCell0->m_objectMetadata].direction);
            drawHeroIcon0 = 1;
            heroYOffset6 = HERO_BOAT_Y_OFFSET;
        } else {
            heroYOffset6 = 0;
            if (newCell0->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_HERO)) {
                savedShowHero = gpGame->GetHero(newCell0->m_objectMetadata);
                flagColorValue = savedShowHero->IsEmbarked()
                                     ? PLAYER_COLOR_NONE
                                     : gpGame->m_players[savedShowHero->m_owner].m_color;
                position = savedShowHero->IsEmbarked() ? static_cast<i8>(ADVMGR_HERO_ICON_BOAT)
                                                       : savedShowHero->m_heroClass;
                savedFrame = GetCursorBaseFrame(savedShowHero->m_direction);
                drawHeroIcon0 = 1;
                if (savedShowHero->IsEmbarked())
                    heroYOffset6 = HERO_BOAT_Y_OFFSET;
            }
        }
        if (drawHeroIcon0) {
            if (savedFrame & HERO_FRAME_MIRROR_FLAG) {
                if (screenX == 0 || screenY <= 1 || screenX == ADVMGR_VIEW_CELL_COUNT - 1
                    || screenY == ADVMGR_VIEW_CELL_COUNT - 1) {
                    FlipClippedIconToBitmap(
                        m_heroIcons[position],
                        gpWindowManager->m_screen,
                        pixelX7 + CELL_PIXELS,
                        pixelY3 + CELL_PIXELS - 1 + heroYOffset6,
                        savedFrame & HERO_FRAME_INDEX_MASK,
                        ICON_DRAW_OFFSET_FULL
                    );
                    if (flagColorValue != PLAYER_COLOR_NONE)
                        FlipClippedIconToBitmap(
                            m_flagIcons[flagColorValue],
                            gpWindowManager->m_screen,
                            pixelX7 + CELL_PIXELS,
                            pixelY3 + CELL_PIXELS - 1 + heroYOffset6,
                            savedFrame & HERO_FRAME_INDEX_MASK,
                            ICON_DRAW_OFFSET_FULL
                        );
                } else {
                    if (m_drawHeroShadows && position != ADVMGR_HERO_ICON_BOAT)
                        FlipDimIconToBitmap(
                            m_boatShadowIcon,
                            gpWindowManager->m_screen,
                            pixelX7 + CELL_PIXELS,
                            pixelY3 + CELL_LAST_PIXEL,
                            savedFrame & HERO_FRAME_INDEX_MASK,
                            ICON_DRAW_OFFSET_FULL
                        );
                    FlipIconToBitmap(
                        m_heroIcons[position],
                        gpWindowManager->m_screen,
                        pixelX7 + CELL_PIXELS,
                        pixelY3 + CELL_PIXELS - 1 + heroYOffset6,
                        savedFrame & HERO_FRAME_INDEX_MASK,
                        ICON_DRAW_OFFSET_FULL
                    );
                    if (flagColorValue != PLAYER_COLOR_NONE)
                        FlipIconToBitmap(
                            m_flagIcons[flagColorValue],
                            gpWindowManager->m_screen,
                            pixelX7 + CELL_PIXELS,
                            pixelY3 + CELL_PIXELS - 1 + heroYOffset6,
                            savedFrame & HERO_FRAME_INDEX_MASK,
                            ICON_DRAW_OFFSET_FULL
                        );
                }
            } else if (screenX == 0 || screenY <= 1 || screenX == ADVMGR_VIEW_CELL_COUNT - 1
                       || screenY == ADVMGR_VIEW_CELL_COUNT - 1) {
                ClippedIconToBitmap(
                    m_heroIcons[position],
                    gpWindowManager->m_screen,
                    pixelX7,
                    pixelY3 + CELL_PIXELS - 1 + heroYOffset6,
                    savedFrame,
                    ICON_DRAW_OFFSET_FULL
                );
                if (flagColorValue != PLAYER_COLOR_NONE)
                    ClippedIconToBitmap(
                        m_flagIcons[flagColorValue],
                        gpWindowManager->m_screen,
                        pixelX7,
                        pixelY3 + CELL_PIXELS - 1 + heroYOffset6,
                        savedFrame & HERO_FRAME_INDEX_MASK,
                        ICON_DRAW_OFFSET_FULL
                    );
            } else {
                if (m_drawHeroShadows && position != ADVMGR_HERO_ICON_BOAT)
                    DimIconToBitmap(
                        m_boatShadowIcon,
                        gpWindowManager->m_screen,
                        pixelX7,
                        pixelY3 + CELL_LAST_PIXEL,
                        savedFrame,
                        ICON_DRAW_OFFSET_FULL
                    );
                IconToBitmap(
                    m_heroIcons[position],
                    gpWindowManager->m_screen,
                    pixelX7,
                    pixelY3 + CELL_PIXELS - 1 + heroYOffset6,
                    savedFrame,
                    ICON_DRAW_OFFSET_FULL
                );
                if (flagColorValue != PLAYER_COLOR_NONE)
                    IconToBitmap(
                        m_flagIcons[flagColorValue],
                        gpWindowManager->m_screen,
                        pixelX7,
                        pixelY3 + CELL_PIXELS - 1 + heroYOffset6,
                        savedFrame & HERO_FRAME_INDEX_MASK,
                        ICON_DRAW_OFFSET_FULL
                    );
            }
        }
        if (m_cursorActive && (newCell0->m_flags & MAP_CELL_HERO_CURSOR) && !m_comboHeroDrawn
            && mapX == m_mapOriginX + ADVMGR_VIEW_CENTER
            && mapY == m_mapOriginY + ADVMGR_VIEW_CENTER) {
            DrawCursor();
            m_comboHeroDrawn = 1;
        }
    }
    if (drawMask & ADVMGR_DRAW_OVERLAY) {
        if (newCell0->m_overlayIndex != MAP_CELL_NO_FRAME) {
            s_drawTileset = newCell0->m_overlayTileset & MAP_CELL_TILESET_MASK;
            IconToBitmap(
                m_objectIcons[s_drawTileset],
                gpWindowManager->m_screen,
                pixelX7,
                pixelY3,
                newCell0->m_overlayIndex,
                ICON_DRAW_OFFSET_FULL
            );
            if (newCell0->m_flags & MAP_CELL_OVERLAY_ANIMATED)
                IconToBitmap(
                    m_objectIcons[s_drawTileset],
                    gpWindowManager->m_screen,
                    pixelX7,
                    pixelY3,
                    newCell0->m_overlayIndex + m_updateMaxX + 1,
                    ICON_DRAW_OFFSET_FULL
                );
        }
        if (newCell0->m_flags & MAP_CELL_OVERLAY_EXTRA)
            IconToBitmap(
                m_objectIcons[newCell0->m_overlayTileset >> MAP_CELL_EXTRA_TILESET_SHIFT],
                gpWindowManager->m_screen,
                pixelX7,
                pixelY3,
                newCell0->m_extraFrame,
                ICON_DRAW_OFFSET_FULL
            );
    }
}

// Returns the map base for any off-grid position.
VA(0x00406d26, 0x58)
mapCell* advManager::GetCell(i16 x, i16 y) {
    if (x < 0 || y < 0 || x >= MAP_CELL_GRID_SIZE || y >= MAP_CELL_GRID_SIZE)
        return m_mapData[0];
    else
        return &m_mapData[x][y];
}

VA(0x00406d7e, 0x4f8)
void advManager::UpdateRadar(i8 updateScreen, i32 partial) {
    i16 y;
    i32 ourFirstX;
    i32 ourLastY;
    i16 x;
    i16 color;
    i16 theOwner;
    i32 curEndX;
    i32 realFirstY;
    mapCell* cellPtrItem;

    if (!partial) {
        ourFirstX = 0;
        realFirstY = 0;
        curEndX = MAP_CELL_GRID_SIZE - 1;
        ourLastY = MAP_CELL_GRID_SIZE - 1;
    } else {
        ourFirstX = m_mapOriginX - 1;
        realFirstY = m_mapOriginY - 1;
        curEndX = m_mapOriginX + ADVMGR_VIEW_CELL_COUNT;
        ourLastY = m_mapOriginY + ADVMGR_VIEW_CELL_COUNT;
        if (ourFirstX < 0)
            ourFirstX = 0;
        if (realFirstY < 0)
            realFirstY = 0;
        if (curEndX > MAP_CELL_GRID_SIZE - 1)
            curEndX = MAP_CELL_GRID_SIZE - 1;
        if (ourLastY > MAP_CELL_GRID_SIZE - 1)
            ourLastY = MAP_CELL_GRID_SIZE - 1;
    }

    if (!gbThisNetHumanPlayer[giCurPlayer])
        return;

    gpAdvManager->m_openState = 0;
    for (x = ourFirstX; x <= curEndX; x++) {
        for (y = realFirstY; y <= ourLastY; y++) {
            if (!(gpGame->m_mapExtra[x][y] & giCurPlayerBit)) {
                m_puzzleIcon->FillToBuffer(
                    x * RADAR_CELL_PIXELS + RADAR_LEFT,
                    y * RADAR_CELL_PIXELS + RADAR_TOP,
                    0,
                    0,
                    ICON_DRAW_NORMAL,
                    ICON_DRAW_OFFSET_FULL
                );
                continue;
            }
            cellPtrItem = &m_mapData[x][y];
            if (MAP_TRIGGER_OBJECT(cellPtrItem->m_triggerType) == MAP_OBJECT_HERO) {
                theOwner = gpGame->m_availableHeroes[cellPtrItem->m_objectMetadata];
                if (theOwner == giCurPlayer)
                    color =
                        gRadarOwnerColor[theOwner < 0 ? 4 : gpGame->m_players[theOwner].m_color];
                else
                    color = gRadarTerrainColor[CELL_TERRAIN(cellPtrItem)];
            } else {
                switch (cellPtrItem->m_objectTileset & MAP_CELL_TILESET_MASK) {
                    case TILESET_TOWN32:
                        theOwner = gpGame->m_townOwners[cellPtrItem->m_objectMetadata];
                        color = gRadarOwnerColor
                            [theOwner < 0 ? 4 : gpGame->m_players[theOwner].m_color];
                        break;
                    case TILESET_RSRC32:
                        switch (cellPtrItem->m_triggerType) {
                            case MAP_OBJECT_TRIGGER(MAP_OBJECT_ALCHEMIST_LAB):
                            case MAP_OBJECT_TRIGGER(MAP_OBJECT_MINE):
                            case MAP_OBJECT_TRIGGER(MAP_OBJECT_SAWMILL):
                            case MAP_EVENT_TRIGGER(MAP_OBJECT_ALCHEMIST_LAB):
                            case MAP_EVENT_TRIGGER(MAP_OBJECT_MINE):
                            case MAP_EVENT_TRIGGER(MAP_OBJECT_SAWMILL):
                                theOwner = gpGame->m_mineOwners[cellPtrItem->m_objectMetadata];
                                color = gRadarOwnerColor
                                    [theOwner < 0 ? 4 : gpGame->m_players[theOwner].m_color];
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
            m_puzzleIcon->FillToBuffer(
                x * RADAR_CELL_PIXELS + RADAR_LEFT,
                y * RADAR_CELL_PIXELS + RADAR_TOP,
                0,
                color,
                ICON_DRAW_NORMAL,
                ICON_DRAW_OFFSET_FULL
            );
        }
    }
    m_puzzleIcon->ClipFillToBuffer(
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
        gpWindowManager->UpdateScreenRegion(
            ourFirstX * RADAR_CELL_PIXELS + RADAR_LEFT,
            realFirstY * RADAR_CELL_PIXELS + RADAR_TOP,
            (curEndX - ourFirstX + 1) * RADAR_CELL_PIXELS,
            (ourLastY - realFirstY + 1) * RADAR_CELL_PIXELS
        );
}

VA(0x00407276, 0x50f)
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
        if (!(gpGame->m_mapExtra[m_mapOriginX + cellX][m_mapOriginY + cellY] & giCurPlayerBit)) {
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
                        gMineNames[gpGame->m_mines[currentCell->m_objectMetadata].type]
                    );
                    break;
                case MAP_OBJECT_RESOURCE:
                    sprintf(
                        gText,
                        "\n\n%s",
                        gResourceNames[currentCell->m_objectIndex - RESOURCE_PILE_OBJECT_BASE]
                    );
                    break;
                case MAP_OBJECT_RESOURCE_SHADOW:
                    sprintf(gText, "\n\n%s", gResourceNames[currentCell->m_objectIndex + 2]);
                    break;
                case MAP_OBJECT_MONSTER:
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
                        "\n\n%s",
                        gObjectNames[currentCell->m_triggerType & MAP_TRIGGER_TYPE_MASK]
                    );
                    break;
            }
        }
    }

    strcpy(savedTextLocal, gText);
    if (giDebugLevel > 0 && currentCell)
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
    gpWindowManager->AddWindow(pWin, WINDOW_Z_ORDER_APPEND, 1);
    gpMouseManager->HideSystemCursor();
    QuickViewWait();
    gpWindowManager->RemoveWindow(pWin);
    delete pWin;
    gpMouseManager->ShowSystemCursor();
}

VA(0x00407785, 0x360)
void advManager::UpdateHeroLocator(i32 locatorSlot, i8 drawWindow, i8 updateScreen) {
    i32 widgetBase;
    i8 whichHero;
    i32 curHero;
    tag_message message;
    i32 i;
    hero* heroPtr;
    i32 moveFrm;

    if (!gbThisNetHumanPlayer[giCurPlayer])
        return;
    if (locatorSlot == LOCATOR_SLOT_CURRENT_HERO) {
        curHero = gpCurPlayer->CurrentHero();
        if (curHero == HERO_ID_NONE)
            return;
        for (i = 0; i < LOCATOR_VISIBLE_COUNT; i++) {
            if (curHero == gpCurPlayer->m_heroIds[gpCurPlayer->m_heroLocatorPage + i])
                locatorSlot = i;
        }
        if (locatorSlot == LOCATOR_SLOT_CURRENT_HERO)
            return;
    }
    widgetBase = locatorSlot * HERO_LOCATOR_WIDGET_STRIDE + HERO_LOCATOR_WIDGET_BASE;
    message.type = MESSAGE_WIDGET;
    whichHero = gpCurPlayer->m_heroIds[gpCurPlayer->m_heroLocatorPage + locatorSlot];
    message.command = WIDGET_COMMAND_SET_COLOR;
    message.id = widgetBase + HERO_LOCATOR_HIGHLIGHT;
    message.value = (whichHero == gpCurPlayer->m_currentHero
                     && gpCurPlayer->m_currentHero != HERO_ID_NONE && !gAllBlack)
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
        heroPtr = gpGame->GetHero(whichHero);
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
            gpWindowManager->UpdateScreenRegion(481, locatorSlot * 32 + 177, 54, 30);
    }
}

VA(0x00407ae5, 0xd3)
void advManager::UpdateHeroLocators(i8 drawWindow, i8 updateScreen) {
    i32 locatorSlot;
    double scrollStep;

    if (!gbThisNetHumanPlayer[giCurPlayer])
        return;

    for (locatorSlot = 0; locatorSlot < LOCATOR_VISIBLE_COUNT; ++locatorSlot)
        UpdateHeroLocator(locatorSlot, 0, 0);

    if (gpCurPlayer->m_heroCount < LOCATOR_PAGE_THRESHOLD) {
        m_scrollLeftButton->m_y = LOCATOR_SCROLL_NO_PAGES_Y;
    } else {
        scrollStep = 73.0 / (gpCurPlayer->m_heroCount - LOCATOR_PAGE_DENOMINATOR_OFFSET);
        m_scrollLeftButton->m_y =
            static_cast<i16>(gpCurPlayer->m_heroLocatorPage * scrollStep + 195.0);
    }
    if (drawWindow)
        m_adventureWindow->DrawWindow(updateScreen);
}

VA(0x00407bb8, 0x225)
void advManager::UpdateTownLocators(i8 drawWindow, i8 updateScreen) {
    i16 i;
    i8 whichTown;
    tag_message msg;
    double step;

    if (!gbThisNetHumanPlayer[giCurPlayer])
        return;
    msg.type = MESSAGE_WIDGET;
    for (i = 0; i < LOCATOR_VISIBLE_COUNT; i++) {
        whichTown = gpCurPlayer->m_townIds[gpCurPlayer->m_townLocatorPage + i];
        msg.command = WIDGET_COMMAND_SET_COLOR;
        msg.id = i + TOWN_LOCATOR_HIGHLIGHT_FIRST;
        msg.value = (gpCurPlayer->m_currentTown != GAME_TOWN_NONE
                     && whichTown == gpCurPlayer->m_currentTown && !gAllBlack)
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
            msg.value = gpGame->GetTown(whichTown)->m_type + LOCATOR_FRAME_TOWN_FIRST;
            if (gpGame->GetTown(whichTown)->m_buildings & (1 << BUILDING_SLOT_CASTLE))
                msg.value += LOCATOR_FRAME_CASTLE_OFFSET;
            m_adventureWindow->BroadcastMessage(msg);
        }
    }
    if (gpCurPlayer->m_townCount < LOCATOR_PAGE_THRESHOLD) {
        m_scrollRightButton->m_y = LOCATOR_SCROLL_NO_PAGES_Y;
    } else {
        step = 74.0 / (gpCurPlayer->m_townCount - LOCATOR_PAGE_DENOMINATOR_OFFSET);
        m_scrollRightButton->m_y = static_cast<i16>(gpCurPlayer->m_townLocatorPage * step + 195.0);
    }
    if (drawWindow)
        m_adventureWindow->DrawWindow(updateScreen);
}

VA(0x00407ddd, 0x143)
void advManager::UpdBottomView(i8 forceUpdate, i8 drawWindow, i8 updateScreen) {
    i8 updated;

    updated = 0;
    gForceUpdate = forceUpdate;
    if (giBottomViewOverride == BOTTOM_VIEW_OVERRIDE_DISABLED)
        return;

    if (giBottomViewOverride > BOTTOM_VIEW_NONE) {
        if (KBTickCount() > giBottomViewOverrideEndTime) {
            giBottomViewOverride = BOTTOM_VIEW_NONE;
        } else {
            switch (giBottomViewOverride) {
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

    if (!gbThisNetHumanPlayer[giCurPlayer] || gAllBlack)
        updated = UpdBottomViewEnemyTurn();
    else if (gpCurPlayer->CurrentHero() == HERO_ID_NONE)
        updated = UpdBottomViewKingdom();
    else
        updated = UpdBottomViewHero();

update_bottom_view:
    if (updated && drawWindow) {
        m_adventureWindow
            ->DrawWindow(0, BOTTOM_VIEW_DRAW_FIRST_WIDGET, BOTTOM_VIEW_DRAW_LAST_WIDGET);
        if (updateScreen)
            gpWindowManager->UpdateScreenRegion(
                BOTTOM_VIEW_PANEL_X,
                BOTTOM_VIEW_PANEL_Y,
                BOTTOM_VIEW_PANEL_WIDTH,
                BOTTOM_VIEW_PANEL_HEIGHT
            );
    }
    forceUpdate = gForceUpdate;
}

VA(0x00407f20, 0x126)
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
    iLastAnimFrame = BOTTOM_VIEW_NO_ANIMATION;
}

VA(0x00408046, 0x52f)
i8 advManager::UpdBottomViewEnemyTurn(void) {
    DATA(0x004a6764)
    static i32 gLastSandAnimTime = 0;
    DATA(0x004a6768)
    static i32 gLastNewSandAnimTime = 0;
    i8 updated;
    tag_message msg;

    updated = 0;
    msg.type = MESSAGE_WIDGET;
    if (gCurBottomView != BOTTOM_VIEW_ENEMY_TURN) {
        updated = 1;
        gForceUpdate = 1;
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
        iLastAnimFrame = m_updateMaxX;
        if (KBTickCount() - gLastNewSandAnimTime > ENEMY_TURN_ANIMATION_DELAY) {
            gLastNewSandAnimTime = KBTickCount();
            gSandAnim++;
            if (gSandAnim >= ENEMY_TURN_SAND_FRAME_LIMIT)
                gSandAnim = ENEMY_TURN_SAND_RESTART_FRAME;
            updated = 1;
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

    if (gForceUpdate || gCurBottomViewEnemy != giCurPlayer) {
        updated = 1;
        gCurBottomViewEnemy = giCurPlayer;
        if (gCurBottomViewEnemy != giCurPlayer)
            gCurHourGlassPhase = 0;
        if (m_bottomViewPrimaryWidgets[ENEMY_TURN_CREST_SLOT + ADVMGR_BOTTOM_VIEW_ICON_FIRST]) {
            msg.command = WIDGET_COMMAND_SET_FRAME;
            msg.id = ENEMY_TURN_CREST_ID;
            msg.value = gpGame->GetPlayerColor(giCurPlayer);
            m_adventureWindow->BroadcastMessage(msg);
        } else {
            m_bottomViewPrimaryWidgets[ENEMY_TURN_CREST_SLOT + ADVMGR_BOTTOM_VIEW_ICON_FIRST] =
                new iconWidget(
                    ENEMY_TURN_CREST_X,
                    ENEMY_TURN_ANIMATION_Y,
                    ENEMY_TURN_ANIMATION_WIDTH,
                    ENEMY_TURN_ANIMATION_HEIGHT,
                    "brcrest.icn",
                    gpGame->GetPlayerColor(giCurPlayer),
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
        updated = 1;
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

VA(0x00408575, 0x372)
i8 advManager::UpdBottomViewNewTurn(void) {
    i32 frameIndex;
    i32 month;
    char* week;
    char* day;

    frameIndex = 0;
    if (!gForceUpdate && gCurBottomView == BOTTOM_VIEW_NEW_TURN)
        return 0;

    ClearBottomView();
    gCurBottomView = BOTTOM_VIEW_NEW_TURN;
    if (gpGame->m_day == 1 && (gpGame->m_month != 1 || gpGame->m_week != 1 || gpGame->m_day != 1))
        frameIndex = gpGame->m_week;

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
        gpGame->m_month,
        localization::Tr("calendar.week.label"),
        gpGame->m_week
    );
    m_bottomViewSecondaryWidgets[0] = new textWidget(
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
    if (!m_bottomViewSecondaryWidgets[0])
        MemError();
    m_adventureWindow->AddWidget(m_bottomViewSecondaryWidgets[0], WINDOW_Z_ORDER_APPEND);

    day = static_cast<char*>(malloc(BOTTOM_VIEW_TEXT_BUFFER_SIZE));
    sprintf(day, "%s: %d", localization::Tr("calendar.day.label"), gpGame->m_day);
    m_bottomViewSecondaryWidgets[0] = new textWidget(
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
    if (!m_bottomViewSecondaryWidgets[0])
        MemError();
    m_adventureWindow->AddWidget(m_bottomViewSecondaryWidgets[0], WINDOW_Z_ORDER_APPEND);
    return 1;
}

VA(0x004088e7, 0x38f)
i8 advManager::UpdBottomViewResMsg(void) {
    i32 oldIconW;
    i32 iconHVal;
    i32 y;
    i32 lineCntNo;
    char* messageText;
    char* prevCountString;
    font* smFont;

    if (!gForceUpdate && gCurBottomView == BOTTOM_VIEW_RESOURCE)
        return 0;

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

    y = 0;
    if (giBottomViewResource < 0) {
        y = RESOURCE_VIEW_MULTILINE_HEIGHT;
        smFont = gpResourceManager->GetFont("smalfont.fnt");
        lineCntNo = smFont->LineLength(gcBottomViewText, BOTTOM_VIEW_PANEL_WIDTH);
        gpResourceManager->Dispose(smFont);
        y -= lineCntNo * RESOURCE_VIEW_LINE_HEIGHT;
    }
    messageText = static_cast<char*>(malloc(strlen(gcBottomViewText) + 1));
    sprintf(messageText, gcBottomViewText);
    m_bottomViewSecondaryWidgets[0] = new textWidget(
        BOTTOM_VIEW_PANEL_X,
        y + RESOURCE_VIEW_TEXT_BASE_Y,
        BOTTOM_VIEW_PANEL_WIDTH,
        RESOURCE_VIEW_TEXT_HEIGHT,
        messageText,
        "smalfont.fnt",
        1,
        BOTTOM_VIEW_TEXT_ID,
        WIDGET_KIND_TEXT
    );
    if (!m_bottomViewSecondaryWidgets[0])
        MemError();
    m_adventureWindow->AddWidget(m_bottomViewSecondaryWidgets[0], WINDOW_Z_ORDER_APPEND);

    if (giBottomViewResource >= 0) {
        if (giBottomViewResource == RESOURCE_GOLD) {
            oldIconW = RESOURCE_VIEW_GOLD_WIDTH;
            iconHVal = RESOURCE_VIEW_GOLD_HEIGHT;
        } else {
            oldIconW = RESOURCE_VIEW_ICON_WIDTH;
            iconHVal = RESOURCE_VIEW_ICON_HEIGHT;
        }
        m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_FOREGROUND] = new iconWidget(
            (BOTTOM_VIEW_PANEL_WIDTH - oldIconW) / 2 + BOTTOM_VIEW_PANEL_X,
            RESOURCE_VIEW_ICON_BOTTOM - iconHVal - RESOURCE_VIEW_ICON_BOTTOM_PADDING,
            oldIconW,
            iconHVal,
            "resource.icn",
            giBottomViewResource,
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

        prevCountString = static_cast<char*>(malloc(BOTTOM_VIEW_COUNT_BUFFER_SIZE));
        sprintf(prevCountString, "%d", giBottomViewResourceQty);
        m_bottomViewSecondaryWidgets[1] = new textWidget(
            RESOURCE_VIEW_COUNT_X,
            RESOURCE_VIEW_COUNT_Y,
            RESOURCE_VIEW_COUNT_WIDTH,
            RESOURCE_VIEW_COUNT_HEIGHT,
            prevCountString,
            "smalfont.fnt",
            1,
            BOTTOM_VIEW_TEXT_ID_2,
            WIDGET_KIND_TEXT
        );
        if (!m_bottomViewSecondaryWidgets[1])
            MemError();
        m_adventureWindow->AddWidget(m_bottomViewSecondaryWidgets[1], WINDOW_Z_ORDER_APPEND);
    }
    return 1;
}

VA(0x00408c76, 0x36e)
i8 advManager::UpdBottomViewKingdom(void) {
    i32 numVillage;
    i32 i;
    i32 nCastles;
    i8 rowY[KINGDOM_VIEW_ENTRY_COUNT];
    u8 textX[KINGDOM_VIEW_ENTRY_COUNT];
    char* countText[KINGDOM_VIEW_ENTRY_COUNT];

    if (!gForceUpdate && gCurBottomView == BOTTOM_VIEW_KINGDOM)
        return 0;

    ClearBottomView();
    gCurBottomView = BOTTOM_VIEW_KINGDOM;
    rowY[RESOURCE_WOOD] = KINGDOM_VIEW_RESOURCE_TEXT_Y;
    rowY[RESOURCE_MERCURY] = KINGDOM_VIEW_RESOURCE_TEXT_Y;
    rowY[RESOURCE_ORE] = KINGDOM_VIEW_RESOURCE_TEXT_Y;
    rowY[RESOURCE_SULFUR] = KINGDOM_VIEW_RESOURCE_TEXT_Y;
    rowY[RESOURCE_CRYSTAL] = KINGDOM_VIEW_RESOURCE_TEXT_Y;
    rowY[RESOURCE_GEMS] = KINGDOM_VIEW_RESOURCE_TEXT_Y;
    rowY[6] = 28;
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

    for (i = 0; i < gpCurPlayer->m_townCount; i++) {
        if (gpGame->m_castleRecs[gpCurPlayer->m_townIds[i]].m_buildings
            & (1 << BUILDING_SLOT_CASTLE))
            nCastles++;
        else
            numVillage++;
    }

    for (i = 0; i < KINGDOM_VIEW_ENTRY_COUNT; i++) {
        countText[i] = static_cast<char*>(malloc(BOTTOM_VIEW_COUNT_BUFFER_SIZE));
        if (i < KINGDOM_VIEW_CASTLE_ENTRY)
            sprintf(countText[i], "%d", gpCurPlayer->m_resources[i]);
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
    return 1;
}

VA(0x00408fe4, 0x585)
i8 advManager::UpdBottomViewHero(void) {
    i16 slotNumPos;
    i8 creatureType;
    i32 j;
    i32 col;
    char* countStrData[ARMY_GROUP_SLOT_COUNT];
    i16 nStacks;
    i16 iCrest;
    hero* curHero;
    i32 y;
    i32 nextX;
    char* newHeroName;

    if (!gForceUpdate && gCurBottomView == BOTTOM_VIEW_HERO)
        return 0;

    ClearBottomView();
    gCurBottomView = BOTTOM_VIEW_HERO;
    curHero = gpGame->GetHero(gpCurPlayer->CurrentHero());
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

    iCrest = gpCurPlayer->Color() * HERO_CLASS_COUNT + curHero->m_heroClass;
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

    newHeroName = static_cast<char*>(malloc(9));
    strcpy(newHeroName, curHero->m_shortName);
    newHeroName[8] = 0;
    m_bottomViewSecondaryWidgets[0] = new textWidget(
        475,
        418,
        66,
        12,
        newHeroName,
        "smalfont.fnt",
        1,
        BOTTOM_VIEW_TEXT_ID,
        WIDGET_KIND_TEXT
    );
    if (!m_bottomViewSecondaryWidgets[0])
        MemError();
    m_adventureWindow->AddWidget(m_bottomViewSecondaryWidgets[0], WINDOW_Z_ORDER_APPEND);

    for (j = 0; j < ARMY_GROUP_SLOT_COUNT; j++) {
        if (curHero->m_army.m_creatureTypes[j] != CREATURE_NONE)
            nStacks++;
    }
    if (nStacks) {
        slotNumPos = 0;
        for (j = 0; j < ARMY_GROUP_SLOT_COUNT; j++) {
            creatureType = curHero->m_army.m_creatureTypes[j];
            if (creatureType != CREATURE_NONE) {
                countStrData[slotNumPos] = static_cast<char*>(malloc(BOTTOM_HERO_LABEL_BYTES));
                sprintf(countStrData[slotNumPos], "%d", curHero->m_army.m_creatureCounts[j]);
                y = slotNumPos <= 2 ? 38 : 3;
                if (slotNumPos == 0) {
                    nextX = nStacks <= 2 ? 77 : 101;
                } else if (slotNumPos == 1) {
                    nextX = nStacks == BOTTOM_HERO_TWO_STACKS ? 28 : 52;
                } else if (slotNumPos == BOTTOM_HERO_SLOT_THIRD) {
                    nextX = 3;
                } else if (slotNumPos == BOTTOM_HERO_SLOT_FOURTH) {
                    nextX = nStacks == BOTTOM_HERO_FOUR_STACKS ? 77 : 101;
                } else {
                    nextX = 52;
                }
                m_bottomViewPrimaryWidgets[slotNumPos + ADVMGR_BOTTOM_VIEW_ICON_FIRST] =
                    new iconWidget(
                        nextX + BOTTOM_VIEW_PANEL_X,
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
                if (gMons32Width[creatureType] < 28 && strlen(countStrData[slotNumPos]) <= 2)
                    col = nextX + 30;
                else
                    col = nextX + gMons32Width[creatureType] + 2;
                m_bottomViewSecondaryWidgets[slotNumPos + ADVMGR_BOTTOM_VIEW_HERO_TEXT_FIRST] =
                    new textWidget(
                        col + BOTTOM_VIEW_PANEL_X,
                        y + 414,
                        strlen(countStrData[slotNumPos]) * BOTTOM_HERO_CHARACTER_WIDTH,
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
    return 1;
}

VA(0x00409569, 0xc9f)
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
    targetHero = gpGame->GetHero(heroId);
    if (targetHero->m_owner == giCurPlayer || m_identifyHeroActive == 1) {
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
    msg.value = gpGame->m_players[targetHero->m_owner].Color() * QUICK_VIEW_FLAG_COLOR_STRIDE;
    win->BroadcastMessage(msg);
    msg.id++;
    msg.value++;
    win->BroadcastMessage(msg);
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

    if (targetHero->m_owner == giCurPlayer || m_identifyHeroActive == 1) {
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
        i16 row2;
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
                row2 = 0;
                break;
            case ARMY_QUICK_FOUR_STACKS:
                topRow = ARMY_QUICK_FIRST_ROW_COUNT;
                row2 = 2;
                break;
            default:
                topRow = ARMY_QUICK_FIRST_ROW_COUNT;
                row2 = 3;
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
        if (row2) {
            stride = HERO_QUICK_ARMY_AREA_WIDTH / row2;
            armyStart = (stride - ARMY_QUICK_ICON_SIZE) / 2 + ARMY_QUICK_AREA_LEFT;
            rowY += ARMY_QUICK_SECOND_ROW_SHIFT;
            for (ii = topRow; ii < topRow + row2; ii++) {
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
    UpdateRadar(1, 0);
    GrabScreen();
    gpWindowManager->AddWindow(win, WINDOW_Z_ORDER_APPEND, 1);
    gpMouseManager->HideSystemCursor();
    QuickViewWait();
    gpWindowManager->RemoveWindow(win);
    delete win;
    gpMouseManager->ShowSystemCursor();
    m_mapOriginX = oldX;
    m_mapOriginY = savedY;
    UpdateRadar(1, 0);
    CompleteDraw(0);
    UpdateScreen(0, 0);
    if (msg.type == MESSAGE_LEFT_BUTTON_DOWN && targetHero->m_owner == giCurPlayer)
        SetHeroContext(targetHero->m_id, 0);
}

VA(0x0040a208, 0xad)
char* advManager::GetArmySizeName(i16 armySize, H1_ENUM_PARAM(ArmySizeNameVariant, i8) grammar) {
    if (giDebugLevel > 0) {
        sprintf(cArmySizeName, "%d", armySize);
        return cArmySizeName;
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

VA(0x0040a2b5, 0xb88)
void advManager::TownQuickView(i8 townId, i8, i16 windowX, i16 windowY) {
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
    townPtr = gpGame->GetTown(townId);
    if (windowX == QUICK_VIEW_AT_LOCATOR) {
        windowX = TOWN_QUICK_DEFAULT_WINDOW_X;
        windowY = TOWN_QUICK_DEFAULT_WINDOW_Y;
    }
    window = new heroWindow(windowX, windowY, "qtown1.bin");
    if (!window)
        MemError();
    if (townPtr->m_owner == giCurPlayer) {
        scouting = TOWN_QUICK_INFORMATION_EXACT;
    } else {
        scouting = gpGame->GetNumThievesGuilds(giCurPlayer);
        if (scouting > TOWN_QUICK_INFORMATION_THIEVES_LAST)
            scouting = TOWN_QUICK_INFORMATION_THIEVES_LAST;
    }
    SetWinText(window, WINDOW_TEXT_TOWN_QUICK_VIEW);

    creatureCount = 0;
    SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_FRAME, QUICK_VIEW_PORTRAIT);
    message.value = townPtr->m_type + TOWN_QUICK_TYPE_FRAME_BASE;
    if (gpGame->GetTown(townId)->m_buildings & (1 << BUILDING_SLOT_CASTLE))
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
        message.value = gpGame->m_players[townPtr->m_owner].Color() * QUICK_VIEW_FLAG_COLOR_STRIDE;
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
        i16 row2;
        i16 stride;
        i8 armySlot;
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
                row2 = 0;
                break;
            case ARMY_QUICK_FOUR_STACKS:
                topRow = ARMY_QUICK_FIRST_ROW_COUNT;
                row2 = 2;
                break;
            default:
                topRow = ARMY_QUICK_FIRST_ROW_COUNT;
                row2 = 3;
                break;
        }
        armySlot = 0;
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
        if (row2) {
            stride = TOWN_QUICK_ARMY_AREA_WIDTH / row2;
            basePos = (stride - ARMY_QUICK_ICON_SIZE) / 2 + ARMY_QUICK_AREA_LEFT;
            curY += ARMY_QUICK_SECOND_ROW_SHIFT;
            for (armyIndex = topRow; armyIndex < topRow + row2; armyIndex++) {
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
    gpWindowManager->AddWindow(window, WINDOW_Z_ORDER_APPEND, 1);
    oldX = m_mapOriginX;
    oldY = m_mapOriginY;
    m_mapOriginX = townPtr->m_x - ADVMGR_VIEW_CENTER;
    m_mapOriginY = townPtr->m_y - ADVMGR_VIEW_CENTER;
    UpdateRadar(1, 0);
    gpMouseManager->HideSystemCursor();
    QuickViewWait();
    gpWindowManager->RemoveWindow(window);
    delete window;
    gpMouseManager->ShowSystemCursor();
    m_mapOriginX = oldX;
    m_mapOriginY = oldY;
    UpdateRadar(1, 0);
    CompleteDraw(0);
    UpdateScreen(0, 0);
    if (message.type == MESSAGE_LEFT_BUTTON_DOWN && townPtr->m_owner == giCurPlayer)
        SetTownContext(townPtr->m_id);
}

VA(0x0040ae3d, 0xd1)
void advManager::RedrawAdvScreen(i32 update) {
    if (!bShowIt)
        return;
    gpResourceManager->GetBackdrop("bord.bmp", gpWindowManager->m_screen);
    SaveAdventureBorder();
    UpdateHeroLocators(0, 0);
    UpdateTownLocators(0, 0);
    UpdBottomView(1, 0, 0);
    m_adventureWindow->DrawWindow(0);
    if (update)
        gpWindowManager->UpdateScreenRegion(0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT);
    UpdateRadar(update, 0);
    CompleteDraw(m_mapOriginX, m_mapOriginY, 0);
    if (update)
        UpdateScreen(0, 0);
}

// DeactivateCurrTown clears the current player's town slot.
VA(0x0040af0e, 0x14)
void advManager::DeactivateCurrTown(void) {
    gpCurPlayer->m_currentTown = GAME_TOWN_NONE;
}

// DeactivateCurrHero demobilizes before clearing the hero slot.
VA(0x0040af22, 0x1c)
void advManager::DeactivateCurrHero(void) {
    DemobilizeCurrHero();
    gpCurPlayer->m_currentHero = HERO_ID_NONE;
}

VA(0x0040af3e, 0x41)
void advManager::MobilizeCurrHero(i32 update) {
    if (gpCurPlayer->m_currentHero == HERO_ID_NONE)
        return;
    if (m_heroContextLocked)
        return;
    SetHeroContext(gpCurPlayer->m_currentHero, update);
}

VA(0x0040af7f, 0x15e)
void advManager::DemobilizeCurrHero(void) {
    if (gpCurPlayer->m_currentHero == HERO_ID_NONE)
        return;
    if (!m_heroContextLocked)
        return;

    m_heroContextLocked = 0;
    hero* heroPointer = gpGame->GetHero(gpCurPlayer->m_currentHero);
    StopCursor(1);
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
    m_cursorActive = 0;
    CompleteDraw(m_mapOriginX, m_mapOriginY, 0);
    UpdateScreen(0, 0);
}

VA(0x0040b0dd, 0x224)
void advManager::SetTownContext(i8 townId) {
    i16 index;
    i8 townNo;
    i8 wasVisible;
    town* townPointer;

    DeactivateCurrHero();
    wasVisible = gpMouseManager->IsVis();
    gpMouseManager->ReallyHidePointer();
    gpCurPlayer->m_currentTown = townId;
    townPointer = gpGame->GetTown(gpCurPlayer->m_currentTown);
    m_mapOriginX = townPointer->m_x - ADVMGR_VIEW_CENTER;
    m_mapOriginY = townPointer->m_y - ADVMGR_VIEW_CENTER;
    townNo = 0;
    for (index = 0; index < gpCurPlayer->m_townCount; index++) {
        if (gpCurPlayer->m_townIds[index] == townId)
            townNo = index;
    }
    if (townNo < gpCurPlayer->m_townLocatorPage)
        gpCurPlayer->m_townLocatorPage = townNo;
    else if (townNo > gpCurPlayer->m_townLocatorPage + (LOCATOR_VISIBLE_COUNT - 1))
        gpCurPlayer->m_townLocatorPage = townNo - (LOCATOR_VISIBLE_COUNT - 1);
    UpdateHeroLocators(1, 1);
    UpdateTownLocators(1, 1);
    HideRoute(0, 0, 1);
    UpdBottomView(1, 1, 1);
    UpdateRadar(1, 0);
    CompleteDraw(m_mapOriginX, m_mapOriginY, 0);
    UpdateScreen(0, 0);
    SetEnvironmentOrigin(m_mapOriginX + ADVMGR_VIEW_CENTER, m_mapOriginY + ADVMGR_VIEW_CENTER, 1);
    townNo = CELL_TERRAIN(GetCell(townPointer->m_x, townPointer->m_y));
    if (townNo != m_currentTerrain) {
        m_currentTerrain = townNo;
        PlayMusic(m_currentTerrain);
    }
    if (wasVisible)
        gpMouseManager->ReallyShowPointer();
    gpInputManager->m_field_0x34a = 1;
    m_lastHoverCell = 0;
}

VA(0x0040b301, 0x38d)
void advManager::SetHeroContext(i8 heroId, i8 update) {
    i8 wasVisible;
    i8 curHeroSlot;
    i16 n;
    mapCell* cellPtrItem;
    hero* heroPtr;

    if (heroId == HERO_ID_NONE)
        return;
    wasVisible = gpMouseManager->IsVis();
    gpMouseManager->ReallyHidePointer();
    DeactivateCurrTown();
    HideRoute(0, 0, 1);
    DeactivateCurrHero();
    m_heroContextLocked = 1;
    gpCurPlayer->m_currentHero = heroId;
    heroPtr = gpGame->GetHero(gpCurPlayer->m_currentHero);
    m_mapOriginX = heroPtr->m_x - ADVMGR_VIEW_CENTER;
    m_mapOriginY = heroPtr->m_y - ADVMGR_VIEW_CENTER;
    m_cursorMapX = m_cursorMapY = ADVMGR_VIEW_CENTER;
    m_previousCursorMapX = m_previousCursorMapY = CURSOR_CELL_NONE;
    m_cursorType =
        heroPtr->IsEmbarked() ? static_cast<i8>(ADVMGR_HERO_ICON_BOAT) : heroPtr->m_heroClass;
    m_cursorDirection = heroPtr->m_direction;
    m_cursorFrame = GetCursorBaseFrame(m_cursorDirection);
    cellPtrItem = GetCell(heroPtr->m_x, heroPtr->m_y);
    cellPtrItem->m_flags |= MAP_CELL_HERO_CURSOR;
    gpGame->RestoreCell(
        heroPtr->m_x,
        heroPtr->m_y,
        heroPtr->m_locationType,
        heroPtr->m_occupiedTown,
        NULL,
        4
    );
    curHeroSlot = 0;
    for (n = 0; n < gpCurPlayer->m_heroCount; n++) {
        if (gpCurPlayer->m_heroIds[n] == heroId)
            curHeroSlot = n;
    }
    if (curHeroSlot < gpCurPlayer->m_heroLocatorPage)
        gpCurPlayer->m_heroLocatorPage = curHeroSlot;
    else if (curHeroSlot > gpCurPlayer->m_heroLocatorPage + (LOCATOR_VISIBLE_COUNT - 1))
        gpCurPlayer->m_heroLocatorPage = curHeroSlot - (LOCATOR_VISIBLE_COUNT - 1);
    UpdateHeroLocators(1, 1);
    UpdateTownLocators(1, 1);
    if (!update && (m_active == 1 || gbThisNetHumanPlayer[giCurPlayer])) {
        Reseed(0, 0);
        SeedTo(heroPtr->m_destinationX, heroPtr->m_destinationY);
        ShowRoute(0, 0, !update);
    }
    UpdBottomView(1, 1, 1);
    m_cursorActive = 1;
    UpdateRadar(1, 0);
    CompleteDraw(m_mapOriginX, m_mapOriginY, 0);
    UpdateScreen(0, 0);
    SetEnvironmentOrigin(m_mapOriginX + ADVMGR_VIEW_CENTER, m_mapOriginY + ADVMGR_VIEW_CENTER, 1);
    curHeroSlot = CELL_TERRAIN(cellPtrItem);
    if (curHeroSlot != m_currentTerrain) {
        m_currentTerrain = curHeroSlot;
        PlayMusic(m_currentTerrain);
    }
    if (!gHeroMoving) {
        if (wasVisible)
            gpMouseManager->ReallyShowPointer();
        gpInputManager->m_field_0x34a = 1;
        m_lastHoverCell = 0;
    }
}

VA(0x0040b68e, 0x245)
void advManager::DoHeroKnob(void) {
    i8 prevPage;
    i16 count;
    i16 pageIndex;
    double scale;
    i16 x;
    i16 y;
    i16 offset;
    tag_message message;

    gpMouseManager->SetCursorShape(4);
    prevPage = gpCurPlayer->m_heroLocatorPage;
    count = gpCurPlayer->m_heroCount;
    scale = 73.0 / (count - LOCATOR_PAGE_DENOMINATOR_OFFSET);
    gpMouseManager->MouseCoords(x, y);
    offset = y - m_scrollLeftButton->m_y;
    gpInputManager->Flush();
    message = gpInputManager->GetEvent();
    while (message.type != MESSAGE_LEFT_BUTTON_UP && message.type != MESSAGE_RIGHT_BUTTON_UP) {
        if (message.type == MESSAGE_MOUSE_MOVE) {
            if (message.y < offset + LOCATOR_SCROLL_BASE_Y)
                message.y = offset + LOCATOR_SCROLL_BASE_Y;
            if (message.y > offset + LOCATOR_SCROLL_BASE_Y + LOCATOR_HERO_SCROLL_SPAN)
                message.y = offset + LOCATOR_SCROLL_BASE_Y + LOCATOR_HERO_SCROLL_SPAN;
            gpMouseManager->Main(message);
            m_scrollLeftButton->m_y = message.y - offset;
            m_adventureWindow->DrawWindow();
            if (count > LOCATOR_VISIBLE_COUNT) {
                pageIndex =
                    static_cast<i16>((m_scrollLeftButton->m_y - LOCATOR_SCROLL_BASE_Y) / scale);
                if (pageIndex != prevPage) {
                    gpCurPlayer->m_heroLocatorPage = pageIndex;
                    if (pageIndex > count - (LOCATOR_VISIBLE_COUNT - 1))
                        pageIndex = count - (LOCATOR_VISIBLE_COUNT - 1);
                    UpdateHeroLocators(0, 1);
                    m_scrollLeftButton->m_y = message.y - offset;
                    m_adventureWindow->DrawWindow();
                    prevPage = pageIndex;
                }
            }
        }
        Process1WindowsMessage();
        message = gpInputManager->GetEvent();
    }
    gpMouseManager->SetCursorShape(6);
    m_scrollLeftButton->m_flags &= ~WIDGET_FLAG_SELECTED;
    UpdateHeroLocators(1, 1);
}

VA(0x0040b8d3, 0x245)
void advManager::DoTownKnob(void) {
    i8 prevPage;
    i16 count;
    i16 pageIndex;
    double scale;
    i16 x;
    i16 y;
    i16 offset;
    tag_message message;

    gpMouseManager->SetCursorShape(4);
    prevPage = gpCurPlayer->m_townLocatorPage;
    count = gpCurPlayer->m_townCount;
    scale = 73.0 / (count - LOCATOR_PAGE_DENOMINATOR_OFFSET);
    gpMouseManager->MouseCoords(x, y);
    offset = y - m_scrollRightButton->m_y;
    gpInputManager->Flush();
    message = gpInputManager->GetEvent();
    while (message.type != MESSAGE_LEFT_BUTTON_UP && message.type != MESSAGE_RIGHT_BUTTON_UP) {
        if (message.type == MESSAGE_MOUSE_MOVE) {
            if (message.y < offset + LOCATOR_SCROLL_BASE_Y)
                message.y = offset + LOCATOR_SCROLL_BASE_Y;
            if (message.y > offset + LOCATOR_SCROLL_BASE_Y + LOCATOR_HERO_SCROLL_SPAN)
                message.y = offset + LOCATOR_SCROLL_BASE_Y + LOCATOR_HERO_SCROLL_SPAN;
            gpMouseManager->Main(message);
            m_scrollRightButton->m_y = message.y - offset;
            m_adventureWindow->DrawWindow();
            if (count > LOCATOR_VISIBLE_COUNT) {
                pageIndex =
                    static_cast<i16>((m_scrollRightButton->m_y - LOCATOR_SCROLL_BASE_Y) / scale);
                if (pageIndex != prevPage) {
                    gpCurPlayer->m_townLocatorPage = pageIndex;
                    if (pageIndex > count - (LOCATOR_VISIBLE_COUNT - 1))
                        pageIndex = count - (LOCATOR_VISIBLE_COUNT - 1);
                    UpdateTownLocators(0, 1);
                    m_scrollRightButton->m_y = message.y - offset;
                    m_adventureWindow->DrawWindow();
                    prevPage = pageIndex;
                }
            }
        }
        Process1WindowsMessage();
        message = gpInputManager->GetEvent();
    }
    gpMouseManager->SetCursorShape(6);
    m_scrollRightButton->m_flags &= ~WIDGET_FLAG_SELECTED;
    UpdateTownLocators(1, 1);
}

VA(0x0040bb18, 0x3a6)
void advManager::ViewPuzzle(void) {
    i32 puzzleX;
    i32 puzzleY;
    i8 visibleCount;
    icon* puzzleIcn;
    heroWindow* pWin;
    i16 j;

    visibleCount = 0;
    PlayMusic(MUSIC_TRACK_PUZZLE);
    gpMouseManager->SetPointer("advmice.mse", ADVENTURE_POINTER_DEFAULT);
    puzzleIcn = gpResourceManager->GetIcon("puzzle.icn");
    for (j = 0; j < PUZZLE_PIECE_COUNT; j++)
        puzzleIcn->DrawToBuffer(0, 0, j, ICON_DRAW_NORMAL, ICON_DRAW_OFFSET_FULL);
    gpWindowManager->UpdateScreenRegion(
        BORDER_EDGE_SIZE,
        BORDER_EDGE_SIZE,
        ADVENTURE_VIEWPORT_INNER_SIZE,
        ADVENTURE_VIEWPORT_INNER_SIZE
    );
    gpWindowManager->SaveFizzleSource(
        BORDER_EDGE_SIZE,
        BORDER_EDGE_SIZE,
        ADVENTURE_VIEWPORT_INNER_SIZE,
        ADVENTURE_VIEWPORT_INNER_SIZE
    );
    pWin = new heroWindow(RADAR_LEFT, RADAR_TOP, "viewpuzl.bin");
    if (!pWin)
        MemError();
    gpWindowManager->AddWindow(pWin, WINDOW_Z_ORDER_APPEND, 1);

    puzzleX = gpGame->m_ultimateArtifactX - ADVMGR_VIEW_CENTER;
    puzzleY = gpGame->m_ultimateArtifactY - ADVMGR_VIEW_CENTER;
    i32 xOff = 0;
    i32 yOff = 0;
    xOff =
        (gpGame->m_ultimateArtifactX + gpGame->m_ultimateArtifactY) % PUZZLE_ALIGNMENT_DIVISOR - 1;
    yOff = (gpGame->m_ultimateArtifactX * PUZZLE_Y_ADJUST_X_FACTOR
            + gpGame->m_ultimateArtifactY * PUZZLE_Y_ADJUST_Y_FACTOR)
               % PUZZLE_ALIGNMENT_DIVISOR
           - 1;
    if ((gpGame->m_ultimateArtifactX + gpGame->m_ultimateArtifactY) % PUZZLE_ALIGNMENT_DIVISOR
        == 1) {
        if (xOff > 0)
            xOff++;
        else if (xOff < 0)
            xOff--;
    } else if ((gpGame->m_ultimateArtifactX + gpGame->m_ultimateArtifactY) % PUZZLE_PARITY_DIVISOR
               == 1) {
        if (yOff > 0)
            yOff++;
        else if (yOff < 0)
            yOff--;
    }
    puzzleX += xOff;
    puzzleY += yOff;
    PuzzleDraw(puzzleX, puzzleY, gpGame->m_ultimateArtifactX, gpGame->m_ultimateArtifactY);

    for (j = 0; j < PUZZLE_PIECE_COUNT; j++) {
        if (!BitTest(gpCurPlayer->m_obelisksVisited, j)) {
            puzzleIcn->DrawToBuffer(0, 0, j, ICON_DRAW_NORMAL, ICON_DRAW_OFFSET_FULL);
            visibleCount++;
        }
    }
    if (visibleCount != PUZZLE_PIECE_COUNT) {
        gpMouseManager->ReallyHidePointer();
        gpWindowManager->FizzleForward(
            BORDER_EDGE_SIZE,
            BORDER_EDGE_SIZE,
            ADVENTURE_VIEWPORT_INNER_SIZE,
            ADVENTURE_VIEWPORT_INNER_SIZE,
            PUZZLE_FIZZLE_TIME
        );
        gpMouseManager->ReallyShowPointer();
    } else {
        gpWindowManager->ReleaseFizzleSource();
    }

    gpWindowManager->DoDialog(pWin, EventWindowHandler, 0);
    delete pWin;
    CompleteDraw(m_mapOriginX, m_mapOriginY, 0);
    UpdateScreen(0, 0);
    UpdateRadar(1, 0);
    PlayMusic(m_currentTerrain);
}

// PuzzleDraw redraws the 15x15 cells itself, overlaying the puzzle's
// visible object/overlay frames and marking the target cell.
VA(0x0040bebe, 0x1e5)
void advManager::PuzzleDraw(i32 left, i32 top, i32 markX, i32 markY) {
    i32 y;
    mapCell* cell;
    i32 x;
    u8 tileset;
    i16 screenX;
    i16 savedDrawY;

    for (y = 0; y < ADVMGR_VIEW_CELL_COUNT; y++) {
        for (x = 0; x < ADVMGR_VIEW_CELL_COUNT; x++) {
            DrawCell(left + x, top + y, x, y, ADVMGR_DRAW_GROUND, 1, 0);
            screenX = x * CELL_PIXELS;
            savedDrawY = y * CELL_PIXELS;
            cell = GetCell(left + x, top + y);
            if (!(cell->m_flags & MAP_CELL_OBJECT_SHADOW_ONLY)
                && cell->m_objectIndex != MAP_CELL_NO_FRAME) {
                tileset = cell->m_objectTileset & MAP_CELL_TILESET_MASK;
                switch (tileset) {
                    case TILESET_OBJ32_04:
                    case TILESET_MTN32:
                    case TILESET_TREE32:
                        IconToBitmap(
                            m_objectIcons[tileset],
                            gpWindowManager->m_screen,
                            screenX,
                            savedDrawY,
                            cell->m_objectIndex,
                            ICON_DRAW_OFFSET_FULL
                        );
                        break;
                    default:
                        break;
                }
            }
            if (cell->m_overlayIndex != MAP_CELL_NO_FRAME) {
                tileset = cell->m_overlayTileset & MAP_CELL_TILESET_MASK;
                switch (tileset) {
                    case TILESET_OBJ32_04:
                    case TILESET_MTN32:
                    case TILESET_TREE32:
                        IconToBitmap(
                            m_objectIcons[tileset],
                            gpWindowManager->m_screen,
                            screenX,
                            savedDrawY,
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
                    gpWindowManager->m_screen,
                    screenX,
                    savedDrawY + ROUTE_DRAW_Y_OFFSET,
                    ROUTE_CELL_DESTINATION - 1,
                    ICON_DRAW_OFFSET_FULL
                );
        }
    }
    DrawAdventureBorder();
}

VA(0x0040c0a3, 0x1a0)
void advManager::CastSpell(i8 spell) {
    hero* caster;
    i32 guardianCount;

    if (gpCurPlayer->CurrentHero() != HERO_ID_NONE)
        caster = gpGame->GetHero(gpCurPlayer->m_currentHero);
    else
        caster = NULL;

    switch (spell) {
        case SPELL_VIEW_MINES:
        case SPELL_VIEW_RESOURCES:
        case SPELL_VIEW_ARTIFACTS:
        case SPELL_VIEW_TOWNS:
        case SPELL_VIEW_HEROES:
        case SPELL_VIEW_ALL:
            ViewWorld(spell, 1, spell == SPELL_VIEW_ALL);
            break;
        case SPELL_IDENTIFY_HERO:
            m_identifyHeroActive = 1;
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
            UpdateHeroLocator(LOCATOR_SLOT_CURRENT_HERO, 1, 1);
            if (spell == SPELL_DIMENSION_DOOR)
                DimensionDoor();
            else
                TownGate();
            break;
        default:
            break;
    }

    if (spell != SPELL_DIMENSION_DOOR && spell != SPELL_TOWN_GATE)
        gpGame->GetHero(gpCurPlayer->m_currentHero)->UseSpell(spell);
}

// The adventure world view (ground6/flag6/spheres icons), opened from
// CastSpell, AdvPanel, Main and the menu handler.
VA(0x0040c243, 0x1062)
void advManager::ViewWorld(i8 spellType, i8 drawAllObjects, i8 drawAllTerrains) {
    icon* flags;
    hero* prevHero;
    i8 ii;
    i8 flip;
    icon* lettersNode;
    i16 index;
    heroWindow* win;
    u16 mask;
    i16 x;
    i16 owner;
    mapCell* cell;
    icon* prevTilesets[VIEW_WORLD_TILESET_COUNT];
    i16 i;
    i16 y;
    i16 screenX;
    icon* spheres;
    i16 workPosY;
    icon* ground;

    gpMouseManager->SetPointer("advmice.mse", ADVENTURE_POINTER_DEFAULT);
    mask = (1 << TILESET_MTN32) | (1 << TILESET_TREE32);
    if (spellType == SPELL_VIEW_TOWNS || spellType == SPELL_VIEW_ALL)
        mask |= 1 << TILESET_TOWN32;
    ground = gpResourceManager->GetIcon("ground6.icn");
    flags = gpResourceManager->GetIcon("flag6.icn");
    spheres = gpResourceManager->GetIcon("spheres.icn");
    lettersNode = gpResourceManager->GetIcon("letters.icn");
    prevHero = NULL;
    for (i = 0; i < VIEW_WORLD_TILESET_COUNT; i++)
        prevTilesets[i] = NULL;
    prevTilesets[TILESET_TREE32] = gpResourceManager->GetIcon("tree6.icn");
    prevTilesets[TILESET_MTN32] = gpResourceManager->GetIcon("mtn6.icn");
    prevTilesets[TILESET_TOWN32] = gpResourceManager->GetIcon("town6.icn");
    if (gpCurPlayer->CurrentHero() != HERO_ID_NONE)
        prevHero = gpGame->GetHero(gpCurPlayer->m_currentHero);
    FillBitmapArea(
        gpWindowManager->m_screen,
        BORDER_EDGE_SIZE,
        BORDER_EDGE_SIZE,
        ADVENTURE_VIEWPORT_INNER_SIZE,
        ADVENTURE_VIEWPORT_INNER_SIZE,
        0
    );

    for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
        for (x = MAP_CELL_GRID_SIZE - 1; x >= 0; x--) {
            cell = GetCell(x, y);
            if ((gpGame->m_mapExtra[x][y] & giCurPlayerBit) || drawAllTerrains
                || (spellType == SPELL_VIEW_TOWNS
                    && MAP_TRIGGER_OBJECT(cell->m_triggerType) == MAP_OBJECT_TOWN)) {
                flip = 0;
                screenX = x * VIEW_WORLD_CELL_PIXELS + VIEW_WORLD_ORIGIN;
                workPosY = y * VIEW_WORLD_CELL_PIXELS + VIEW_WORLD_ORIGIN;
                index = cell->m_tileIndex / (1 << VIEW_WORLD_GROUND_TILE_SHIFT);
                if (cell->m_flags & MAP_CELL_GROUND_FLIP_HORIZONTAL)
                    flip = 1;
                if (cell->m_flags & MAP_CELL_GROUND_FLIP_VERTICAL)
                    index += VIEW_WORLD_GROUND_FLIPPED_FRAMES;
                ground->DrawToBuffer(
                    screenX + (flip == 1 ? VIEW_WORLD_CELL_PIXELS - 1 : 0),
                    workPosY,
                    index,
                    flip,
                    ICON_DRAW_OFFSET_FULL
                );
                if (cell->m_objectIndex != MAP_CELL_NO_FRAME) {
                    ii = cell->m_objectTileset & MAP_CELL_TILESET_MASK;
                    if ((1 << ii) & mask)
                        prevTilesets[ii]->DrawToBuffer(
                            screenX,
                            workPosY,
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
            workPosY = y * VIEW_WORLD_CELL_PIXELS + VIEW_WORLD_ORIGIN;
            if ((drawAllObjects || (gpGame->m_mapExtra[x][y] & giCurPlayerBit))
                && (cell->m_triggerType & MAP_TRIGGER_EVENT)) {
                switch (spellType) {
                    case SPELL_VIEW_ALL:
                        if (cell->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_ARTIFACT))
                            flags->DrawToBuffer(
                                screenX,
                                workPosY,
                                VIEW_WORLD_FLAG_ARTIFACT,
                                ICON_DRAW_NORMAL,
                                ICON_DRAW_OFFSET_FULL
                            );
                        if (cell->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_TOWN)) {
                            owner = gpGame->m_townOwners[cell->m_objectMetadata];
                            if (owner >= 0) {
                                index = gpGame->m_players[owner].m_color;
                                flags->DrawToBuffer(
                                    screenX - VIEW_WORLD_TOWN_FLAG_LEFT,
                                    workPosY,
                                    index,
                                    ICON_DRAW_FLIPPED,
                                    ICON_DRAW_OFFSET_FULL
                                );
                                flags->DrawToBuffer(
                                    screenX + VIEW_WORLD_TOWN_FLAG_RIGHT,
                                    workPosY,
                                    index,
                                    ICON_DRAW_NORMAL,
                                    ICON_DRAW_OFFSET_FULL
                                );
                            }
                        } else if (cell->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_HERO)
                                   && gpGame->m_heroRecs[cell->m_objectMetadata].m_locationType
                                          == MAP_EVENT_TRIGGER(MAP_OBJECT_TOWN)) {
                            owner = gpGame->m_townOwners[gpGame->m_heroRecs[cell->m_objectMetadata]
                                                             .m_occupiedTown];
                            if (owner >= 0) {
                                index = gpGame->m_players[owner].m_color;
                                flags->DrawToBuffer(
                                    screenX - VIEW_WORLD_TOWN_FLAG_LEFT,
                                    workPosY,
                                    index,
                                    ICON_DRAW_FLIPPED,
                                    ICON_DRAW_OFFSET_FULL
                                );
                                flags->DrawToBuffer(
                                    screenX + VIEW_WORLD_TOWN_FLAG_RIGHT,
                                    workPosY,
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
                                owner = gpGame->m_mineOwners[cell->m_objectMetadata];
                                index = owner < 0 ? PLAYER_COLOR_NEUTRAL
                                                  : gpGame->m_players[owner].m_color;
                                spheres->DrawToBuffer(
                                    screenX,
                                    workPosY,
                                    index,
                                    ICON_DRAW_NORMAL,
                                    ICON_DRAW_OFFSET_FULL
                                );
                                lettersNode->DrawToBuffer(
                                    screenX,
                                    workPosY,
                                    gpGame->m_mines[cell->m_objectMetadata].type,
                                    ICON_DRAW_NORMAL,
                                    ICON_DRAW_OFFSET_FULL
                                );
                                break;
                            case MAP_OBJECT_HERO:
                                switch (MAP_TRIGGER_OBJECT(gpGame->m_heroRecs[cell->m_objectMetadata].m_locationType)) {
                                    case MAP_OBJECT_ALCHEMIST_LAB:
                                    case MAP_OBJECT_MINE:
                                    case MAP_OBJECT_SAWMILL:
                                        owner = gpGame->m_mineOwners
                                                    [gpGame->m_heroRecs[cell->m_objectMetadata]
                                                         .m_occupiedTown];
                                        index = owner < 0 ? PLAYER_COLOR_NEUTRAL
                                                          : gpGame->m_players[owner].m_color;
                                        spheres->DrawToBuffer(
                                            screenX,
                                            workPosY,
                                            index,
                                            ICON_DRAW_NORMAL,
                                            ICON_DRAW_OFFSET_FULL
                                        );
                                        lettersNode->DrawToBuffer(
                                            screenX,
                                            workPosY,
                                            gpGame->m_mines[cell->m_objectMetadata].type,
                                            ICON_DRAW_NORMAL,
                                            ICON_DRAW_OFFSET_FULL
                                        );
                                        break;
                                    default:
                                        break;
                                }
                        }
                        if (cell->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_HERO)) {
                            owner = gpGame->m_availableHeroes[cell->m_objectMetadata];
                            if (owner >= 0) {
                                index = gpGame->m_players[owner].m_color;
                                flags->DrawToBuffer(
                                    screenX,
                                    workPosY,
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
                                owner = gpGame->m_mineOwners[cell->m_objectMetadata];
                                index = owner < 0 ? PLAYER_COLOR_NEUTRAL
                                                  : gpGame->m_players[owner].m_color;
                                spheres->DrawToBuffer(
                                    screenX,
                                    workPosY,
                                    index,
                                    ICON_DRAW_NORMAL,
                                    ICON_DRAW_OFFSET_FULL
                                );
                                lettersNode->DrawToBuffer(
                                    screenX,
                                    workPosY,
                                    gpGame->m_mines[cell->m_objectMetadata].type,
                                    ICON_DRAW_NORMAL,
                                    ICON_DRAW_OFFSET_FULL
                                );
                                break;
                            case MAP_OBJECT_HERO:
                                switch (MAP_TRIGGER_OBJECT(gpGame->m_heroRecs[cell->m_objectMetadata].m_locationType)) {
                                    case MAP_OBJECT_ALCHEMIST_LAB:
                                    case MAP_OBJECT_MINE:
                                    case MAP_OBJECT_SAWMILL:
                                        owner = gpGame->m_mineOwners
                                                    [gpGame->m_heroRecs[cell->m_objectMetadata]
                                                         .m_occupiedTown];
                                        index = owner < 0 ? PLAYER_COLOR_NEUTRAL
                                                          : gpGame->m_players[owner].m_color;
                                        spheres->DrawToBuffer(
                                            screenX,
                                            workPosY,
                                            index,
                                            ICON_DRAW_NORMAL,
                                            ICON_DRAW_OFFSET_FULL
                                        );
                                        lettersNode->DrawToBuffer(
                                            screenX,
                                            workPosY,
                                            gpGame->m_mines[cell->m_objectMetadata].type,
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
                                workPosY,
                                PLAYER_COLOR_NEUTRAL,
                                ICON_DRAW_NORMAL,
                                ICON_DRAW_OFFSET_FULL
                            );
                            lettersNode->DrawToBuffer(
                                screenX - VIEW_WORLD_RESOURCE_X_SHIFT,
                                workPosY,
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
                                workPosY,
                                VIEW_WORLD_FLAG_ARTIFACT,
                                ICON_DRAW_NORMAL,
                                ICON_DRAW_OFFSET_FULL
                            );
                        break;
                    case SPELL_VIEW_TOWNS:
                        if (cell->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_TOWN)) {
                            owner = gpGame->m_townOwners[cell->m_objectMetadata];
                            if (owner >= 0) {
                                index = gpGame->m_players[owner].m_color;
                                flags->DrawToBuffer(
                                    screenX - VIEW_WORLD_TOWN_FLAG_LEFT,
                                    workPosY,
                                    index,
                                    ICON_DRAW_FLIPPED,
                                    ICON_DRAW_OFFSET_FULL
                                );
                                flags->DrawToBuffer(
                                    screenX + VIEW_WORLD_TOWN_FLAG_RIGHT,
                                    workPosY,
                                    index,
                                    ICON_DRAW_NORMAL,
                                    ICON_DRAW_OFFSET_FULL
                                );
                            }
                        } else if (cell->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_HERO)
                                   && gpGame->m_heroRecs[cell->m_objectMetadata].m_locationType
                                          == MAP_EVENT_TRIGGER(MAP_OBJECT_TOWN)) {
                            owner = gpGame->m_townOwners[gpGame->m_heroRecs[cell->m_objectMetadata]
                                                             .m_occupiedTown];
                            if (owner >= 0) {
                                index = gpGame->m_players[owner].m_color;
                                flags->DrawToBuffer(
                                    screenX - VIEW_WORLD_TOWN_FLAG_LEFT,
                                    workPosY,
                                    index,
                                    ICON_DRAW_FLIPPED,
                                    ICON_DRAW_OFFSET_FULL
                                );
                                flags->DrawToBuffer(
                                    screenX + VIEW_WORLD_TOWN_FLAG_RIGHT,
                                    workPosY,
                                    index,
                                    ICON_DRAW_NORMAL,
                                    ICON_DRAW_OFFSET_FULL
                                );
                            }
                        }
                        break;
                    case SPELL_VIEW_HEROES:
                        if (cell->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_HERO)) {
                            owner = gpGame->m_availableHeroes[cell->m_objectMetadata];
                            if (owner >= 0) {
                                index = gpGame->m_players[owner].m_color;
                                flags->DrawToBuffer(
                                    screenX,
                                    workPosY,
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
            if (prevHero && prevHero->m_x == x && prevHero->m_y == y)
                flags->DrawToBuffer(
                    screenX,
                    workPosY,
                    VIEW_WORLD_FLAG_CURRENT_HERO,
                    ICON_DRAW_NORMAL,
                    ICON_DRAW_OFFSET_FULL
                );
        }
        for (x = MAP_CELL_GRID_SIZE - 1; x >= 0; x--) {
            cell = GetCell(x, y);
            if ((gpGame->m_mapExtra[x][y] & giCurPlayerBit) || drawAllTerrains
                || (cell->m_triggerType == MAP_OBJECT_TRIGGER(MAP_OBJECT_TOWN) && spellType == SPELL_VIEW_TOWNS)) {
                screenX = x * VIEW_WORLD_CELL_PIXELS + VIEW_WORLD_ORIGIN;
                workPosY = y * VIEW_WORLD_CELL_PIXELS + VIEW_WORLD_ORIGIN;
                if (cell->m_overlayIndex != MAP_CELL_NO_FRAME) {
                    ii = cell->m_overlayTileset & MAP_CELL_TILESET_MASK;
                    if ((1 << ii) & mask)
                        prevTilesets[ii]->DrawToBuffer(
                            screenX,
                            workPosY,
                            cell->m_overlayIndex,
                            ICON_DRAW_NORMAL,
                            ICON_DRAW_OFFSET_FULL
                        );
                }
            }
        }
    }

    gpWindowManager->UpdateScreenRegion(
        BORDER_EDGE_SIZE,
        BORDER_EDGE_SIZE,
        ADVENTURE_VIEWPORT_INNER_SIZE,
        ADVENTURE_VIEWPORT_INNER_SIZE
    );
    sprintf(gText, "view-%02d.bin", spellType - SPELL_VIEW_MINES);
    win = new heroWindow(RADAR_LEFT, RADAR_TOP, gText);
    if (!win)
        MemError();
    gpWindowManager->DoDialog(win, TrueFalseDialogHandler, 0);
    delete win;
    UpdateRadar(1, 0);
    for (i = 0; i < VIEW_WORLD_TILESET_COUNT; i++) {
        if (prevTilesets[i])
            gpResourceManager->Dispose(prevTilesets[i]);
    }
    gpResourceManager->Dispose(ground);
    gpResourceManager->Dispose(flags);
    gpResourceManager->Dispose(spheres);
    gpResourceManager->Dispose(lettersNode);
    RedrawAdvScreen(1);
}

// Refresh the saved screen copy with the pointer hidden.
VA(0x0040d2a5, 0x36)
void advManager::GrabScreen(void) {
    gpMouseManager->ReallyHidePointer();
    GrabScreenBitmap(gpWindowManager->m_screen, 0, 0);
    gpMouseManager->ReallyShowPointer();
}

// The control panel and system options in one cpanel.bin dialog, which also
// applies the walk-speed sample set and saves changed preferences.
VA(0x0040d2db, 0x2eb)
i16 advManager::ControlPanel(void) {
    tag_message message;
    i32 anyMobilized;
    i8 oldSpeedState;
    i32 gameCommand;
    i32 n;

    TrimLoopingSounds(ADVMGR_ACTIVE_SOUND_COUNT);
    gpMouseManager->SetPointer("advmice.mse", ADVENTURE_POINTER_DEFAULT);
    gameCommand = MAIN_MENU_NO_COMMAND;
    oldSpeedState = gConfig.walkSpeed;
    gFreshSave = 0;
    anyMobilized = m_heroContextLocked;
    bPrefsChanged = 0;
    DemobilizeCurrHero();
    gPanel = new heroWindow(160, 10, "cpanel.bin");
    if (gPanel == NULL)
        MemError();
    SetWinText(gPanel, WINDOW_TEXT_CONTROL_PANEL);
    if (gRemoteOn) {
        message.type = MESSAGE_WIDGET;
        message.id = CONTROL_NEW_GAME;
        message.command = WIDGET_COMMAND_SET_FLAGS;
        message.value = WIDGET_FLAG_DIM_REQUEST;
        gPanel->BroadcastMessage(message);
        message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.value = WIDGET_FLAG_ENABLED;
        gPanel->BroadcastMessage(message);
        message.id = CONTROL_LOAD_GAME;
        message.command = WIDGET_COMMAND_SET_FLAGS;
        message.value = WIDGET_FLAG_DIM_REQUEST;
        gPanel->BroadcastMessage(message);
        message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.value = WIDGET_FLAG_ENABLED;
        gPanel->BroadcastMessage(message);
    }
    UpdateCPanel(1);
    gpWindowManager->DoDialog(gPanel, CPanelHandler, 0);
    delete gPanel;
    switch (gpWindowManager->m_dialogResult) {
        case CONTROL_NEW_GAME:
        case CONTROL_LOAD_GAME:
        case CONTROL_QUIT:
            gameCommand = gpWindowManager->m_dialogResult;
            break;
        case CONTROL_SCENARIO_INFO:
            if (gpGame->m_campaignType > 0)
                gpGame->ShowCampaignInfo(gpGame->m_campaignScenario, 1, 0);
            else
                gpGame->ShowScenInfo();
            break;
        case CONTROL_SAVE_GAME:
            SaveGame();
            break;
    }
    if (gConfig.walkSpeed != oldSpeedState) {
        for (n = 0; n < ADVMGR_CURSOR_SAMPLE_COUNT; n++) {
            if (m_cursorSamples[n]) {
                gpResourceManager->Dispose(m_cursorSamples[n]);
                m_cursorSamples[n] = NULL;
            }
        }
        GetCursorSampleSet(gConfig.walkSpeed);
    }
    if (bPrefsChanged)
        WritePrefs();
    if (anyMobilized)
        MobilizeCurrHero(0);
    if (gameCommand != MAIN_MENU_NO_COMMAND) {
        gGameCommand = gameCommand;
        return 1;
    }
    return 0;
}

// Updates the six control-panel options.
VA(0x0040d5c6, 0x207)
void UpdateCPanel(i8 initialDraw) {
    tag_message message;

    SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_FRAME, CONTROL_MUSIC_VOLUME);
    message.value = gConfig.musicVolume ? CPANEL_FRAME_MUSIC_ON : CPANEL_FRAME_MUSIC_OFF;
    gPanel->BroadcastMessage(message);
    message.id = CONTROL_SOUND_VOLUME;
    message.value = gConfig.soundVolume ? CPANEL_FRAME_SOUND_ON : CPANEL_FRAME_SOUND_OFF;
    gPanel->BroadcastMessage(message);
    message.id = CONTROL_WALK_SPEED;
    message.value = gConfig.walkSpeed + CPANEL_FRAME_WALK_SPEED_FIRST;
    gPanel->BroadcastMessage(message);
    message.id = CONTROL_MUSIC_SOURCE;
    message.value = (gConfig.musicSource ? CPANEL_MUSIC_LABEL_CD : CPANEL_MUSIC_LABEL_LOCAL)
                    + CPANEL_FRAME_MUSIC_SOURCE_FIRST;
    gPanel->BroadcastMessage(message);
    message.id = CONTROL_SHOW_ROUTE;
    message.value = gConfig.showRoute + CPANEL_FRAME_SHOW_ROUTE_FIRST;
    gPanel->BroadcastMessage(message);
    message.id = CONTROL_SHOW_ENEMY_MOVES;
    if (gRemoteOn)
        message.value = CPANEL_FRAME_ENEMY_MOVES_FIRST;
    else
        message.value = 1 - gConfig.blackoutComputer + CPANEL_FRAME_ENEMY_MOVES_FIRST;
    gPanel->BroadcastMessage(message);
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = CONTROL_MUSIC_VOLUME_TEXT;
    message.text = onOffText[gConfig.musicVolume];
    gPanel->BroadcastMessage(message);
    message.id = CONTROL_SOUND_VOLUME_TEXT;
    message.text = onOffText[gConfig.soundVolume];
    gPanel->BroadcastMessage(message);
    message.id = CONTROL_WALK_SPEED_TEXT;
    message.text = walkSpeedText[gConfig.walkSpeed];
    gPanel->BroadcastMessage(message);
    message.id = CONTROL_MUSIC_SOURCE_TEXT;
    message.text =
        musicQualityText[gConfig.musicSource ? CPANEL_MUSIC_LABEL_CD : CPANEL_MUSIC_LABEL_LOCAL];
    gPanel->BroadcastMessage(message);
    message.id = CONTROL_SHOW_ROUTE_TEXT;
    message.text = onOffText[gConfig.showRoute];
    gPanel->BroadcastMessage(message);
    message.id = CONTROL_SHOW_ENEMY_MOVES_TEXT;
    message.text = onOffText[1 - gConfig.blackoutComputer];
    gPanel->BroadcastMessage(message);
    if (!initialDraw)
        gPanel->MoveWindow(0, 0);
}

VA(0x0040d7cd, 0x205)
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
    gpAdvManager->DisableButtons();
    gpMouseManager->SetPointer("advmice.mse", ADVENTURE_POINTER_DEFAULT);
    for (player = 0; player < GAME_PLAYER_COUNT; player++)
        if (!gpGame->m_playerDead[player] && gbHumanPlayer[player])
            humans++;
    if (gCampaignChoice > 0) {
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
    res = gpExec->DoDialog(newFileReq);
    if (res == DIALOG_BUTTON_2) {
        success = 1;
        gFreshSave = 1;
        success = gpGame->SaveGame(gLastFilename, 0);
        if (success)
            NormalDialog(localization::Tr("adventure.save.success"), NORMAL_DIALOG_TYPE_OK, 0xb1);
    }
    delete newFileReq;
    gpAdvManager->EnableButtons();
    return success;
}

// The control-panel handler, including option cycling.
VA(0x0040d9d2, 0x49e)
H1_ENUM_RETURN(MessageDispatchResult, i16) CPanelHandler(struct tag_message& message) {
    i8 anyChanged = 0;
    char question[120];
    i8 handled = 0;
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
                if (helpIndex >= 0)
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
                            handled = 1;
                            if (!gFreshSave) {
                                NormalDialog(question, NORMAL_DIALOG_TYPE_YES_NO, 0xb1, 0x50);
                                if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CANCEL)
                                    handled = 0;
                            }
                            break;
                        case CONTROL_SAVE_GAME:
                            handled = 1;
                            break;
                        case CONTROL_SCENARIO_INFO:
                        case PANEL_CLOSE_WIDGET:
                            handled = 1;
                            break;
                    }
                    break;
                case WIDGET_NOTIFY_SELECT:
                    switch (message.id) {
                        case CONTROL_MUSIC_VOLUME:
                            gConfig.musicVolume =
                                (gConfig.musicVolume + 1) % (SOUND_VOLUME_LAST + 1);
                            SetMusicVolume(gConfig.musicVolume);
                            anyChanged = 1;
                            bPrefsChanged = 1;
                            break;
                        case CONTROL_SOUND_VOLUME:
                            gConfig.soundVolume =
                                (gConfig.soundVolume + 1) % (SOUND_VOLUME_LAST + 1);
                            SetEffectsVolume(gConfig.soundVolume);
                            anyChanged = 1;
                            bPrefsChanged = 1;
                            break;
                        case CONTROL_WALK_SPEED:
                            ++gConfig.walkSpeed;
                            gConfig.walkSpeed %= WALK_SPEED_COUNT;
                            anyChanged = 1;
                            bPrefsChanged = 1;
                            break;
                        case CONTROL_MUSIC_SOURCE:
                            if (gConfig.musicSource) {
                                gConfig.musicSource = SOUND_MUSIC_SOURCE_DIGITAL;
                            } else {
                                gConfig.musicSource = SOUND_MUSIC_SOURCE_CD;
                            }
                            SetMusicSource(gConfig.musicSource != 0);
                            anyChanged = 1;
                            bPrefsChanged = 1;
                            break;
                        case CONTROL_SHOW_ROUTE:
                            gConfig.showRoute = 1 - gConfig.showRoute;
                            anyChanged = 1;
                            bPrefsChanged = 1;
                            break;
                        case CONTROL_SHOW_ENEMY_MOVES:
                            if (!gRemoteOn) {
                                gConfig.blackoutComputer = 1 - gConfig.blackoutComputer;
                                anyChanged = 1;
                                bPrefsChanged = 1;
                            }
                            break;
                    }
                    break;
            }
        }
    }
    if (anyChanged)
        UpdateCPanel(0);
    if (handled) {
        FINISH_DIALOG_MESSAGE(message);
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

VA(0x0040de70, 0x95)
void advManager::CheckCastSpell(void) {
    if (gpCurPlayer->CurrentHero() != HERO_ID_NONE) {
        MobilizeCurrHero(0);
        CompleteDraw(0);
        UpdateScreen(0, 0);
        GrabScreen();
        gpMouseManager->SetPointer("advmice.mse", ADVENTURE_POINTER_DEFAULT);
        CastSpell(gpGame->ViewSpells(
            gpGame->GetHero(gpCurPlayer->m_currentHero),
            SPELL_TYPE_ADVENTURE,
            NullHandler,
            0
        ));
    }
}

VA(0x0040df05, 0x1ce)
void advManager::AdvPanel(void) {
    heroWindow* adventurePanel;
    {

        TrimLoopingSounds(ADVMGR_ACTIVE_SOUND_COUNT);
        gpMouseManager->SetPointer("advmice.mse", ADVENTURE_POINTER_DEFAULT);
        i32 heroWasMobilized = m_heroContextLocked;
        struct tag_message message;
        DemobilizeCurrHero();

        adventurePanel = new heroWindow(160, 40, "apanel.bin");
        if (adventurePanel == NULL)
            MemError();
        if (gpCurPlayer->CurrentHero() == HERO_ID_NONE) {
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

        gpWindowManager->DoDialog(adventurePanel, APanelHandler, 0);
        delete adventurePanel;
        switch (gpWindowManager->m_dialogResult) {
            case PANEL_CAST_SPELL:
                CheckCastSpell();
                break;
            case PANEL_SEARCH:
                ProcessSearch(ADVMGR_SEARCH_VIEW_CENTER, ADVMGR_SEARCH_VIEW_CENTER);
                break;
            case PANEL_VIEW_WORLD:
                ViewWorld(SPELL_VIEW_ALL, 0, 0);
                break;
            case PANEL_VIEW_PUZZLE:
                ViewPuzzle();
                break;
        }

        if (heroWasMobilized)
            MobilizeCurrHero(0);
    }
}

VA(0x0040e0d3, 0x150)
H1_ENUM_RETURN(MessageDispatchResult, i16) APanelHandler(struct tag_message& message) {
    i8 handled = 0;
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
                            handled = 1;
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

VA(0x0040e223, 0x29e)
H1_ENUM_RETURN(MessageDispatchResult, i16) DimensionDoorHandler(struct tag_message& message) {
    i8 result;
    i16 mouseX;
    i16 mouseY;
    mapCell* cell;

    if (glTimers[ADVENTURE_FRAME_TIMER_SLOT] < KBTickCount()) {
        gpAdvManager->CompleteDraw(gpAdvManager->m_mapOriginX, gpAdvManager->m_mapOriginY, 0);
        gpAdvManager->UpdateScreen(0, 0);
    }
    result = 0;
    switch (message.type) {
        case MESSAGE_WIDGET:
            switch (message.command) {
                case WIDGET_NOTIFY_SELECT:
                    switch (message.id) {
                        case DIMENSION_DOOR_FIRST_BUTTON:
                        case DIMENSION_DOOR_LAST_BUTTON:
                            if (message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON) {
                            } else if (gpWindowManager->m_dialogResult == TRAVEL_DIALOG_ACCEPT) {
                                result = 1;
                            }
                            break;
                        default:
                            break;
                    }
                    break;
                case WIDGET_COMMAND_HOVER:
                    switch (message.id) {
                        case DIMENSION_DOOR_LAST_BUTTON:
                            gpWindowManager->m_dialogResult = TRAVEL_DIALOG_REJECT;
                            gpMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
                            break;
                        case ADVENTURE_CONTROL_MAP_VIEW:
                            gpMouseManager->MouseCoords(mouseX, mouseY);
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
                            if (gpAdvManager->m_lastHoverCell != mouseX
                                || gpAdvManager->m_hoverCellY != mouseY) {
                                gpAdvManager->m_lastHoverCell = mouseX;
                                gpAdvManager->m_hoverCellY = mouseY;
                                cell = gpAdvManager->GetCell(
                                    gpAdvManager->m_mapOriginX + mouseX,
                                    gpAdvManager->m_mapOriginY + mouseY
                                );
                                if ((cell->m_triggerType & MAP_TRIGGER_EVENT)
                                    || (cell->m_secondaryTrigger & MAP_CELL_SECONDARY_BLOCKED)) {
                                    gpWindowManager->m_dialogResult = TRAVEL_DIALOG_REJECT;
                                    gpMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
                                } else {
                                    gpWindowManager->m_dialogResult = TRAVEL_DIALOG_ACCEPT;
                                    gpMouseManager->SetPointer(ADVENTURE_POINTER_MOVE);
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
                            gpWindowManager->m_dialogResult = TRAVEL_DIALOG_REJECT;
                            result = 1;
                            break;
                    }
                    break;
            }
            break;
    }
    if (result) {
        message.command = message.id = WIDGET_COMMAND_DIALOG_SELECT;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// Returns the redraw flag.
VA(0x0040e4c1, 0xb48)
i8 advManager::ComboDraw(i16 originX, i16 originY, i8 animate) {
    DATA(0x004a676c)
    static i32 gFrameCount = 0;
    i32 updateCount;
    i32 drawY;
    i32 drawX;
    mapCell* cell;

    PollSound();
    if (!bShowIt)
        return 0;
    if (m_forceCompleteDraw) {
        CompleteDraw(originX, originY, 0);
        return 1;
    }
    if (animate) {
        gFrameCount += giFrameStep;
        if (gFrameCount < COMBO_FRAME_LIMIT) {
            Process1WindowsMessage();
            if (glTimers[ADVENTURE_FRAME_TIMER_SLOT] < KBTickCount())
                glTimers[ADVENTURE_FRAME_TIMER_SLOT] = KBTickCount() + TIMER_DELAY;
            PollSound();
            return 0;
        } else {
            gFrameCount = 0;
        }
    }

    m_previousOriginX = m_mapOriginX;
    m_previousOriginY = m_mapOriginY;
    memset(bComboDraw, 0, COMBO_CLEAR_BYTES);
    m_comboHeroDrawn = 0;

    for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++) {
        for (drawY = 0; drawY < ADVMGR_VIEW_CELL_COUNT; drawY++) {
            if (originX + drawX >= 0 && originX + drawX < MAP_CELL_GRID_SIZE && originY + drawY >= 0
                && originY + drawY < MAP_CELL_GRID_SIZE) {
                cell = GetCell(originX + drawX, originY + drawY);
                if (cell->m_flags & (MAP_CELL_OBJECT_ANIMATED | MAP_CELL_OVERLAY_ANIMATED))
                    ++bComboDraw[drawX][drawY];
                if (cell->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_MONSTER)) {
                    ++bComboDraw[drawX][drawY];
                    if (GetCloudLookup(drawX + originX, drawY + originY)) {
                        bComboDraw[drawX + 1][drawY] += COMBO_CLOUD_MARK;
                        if (drawY >= 1) {
                            bComboDraw[drawX][drawY - 1] += COMBO_CLOUD_MARK;
                            bComboDraw[drawX + 1][drawY - 1] += COMBO_CLOUD_MARK;
                        }
                    } else {
                        ++bComboDraw[drawX + 1][drawY];
                        if (drawY >= 1) {
                            ++*(bComboDraw[drawX] + drawY - 1);
                            ++bComboDraw[drawX + 1][drawY - 1];
                        }
                    }
                }
                if (cell->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_HERO)
                    || cell->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_SHIP)) {
                    ++bComboDraw[drawX][drawY];
                    if (GetCloudLookup(drawX + originX, drawY + originY)) {
                        bComboDraw[drawX + 1][drawY] += COMBO_CLOUD_MARK;
                        bComboDraw[drawX][drawY + 1] += COMBO_CLOUD_MARK;
                        if (drawY >= 1)
                            bComboDraw[drawX][drawY - 1] += COMBO_CLOUD_MARK;
                        if (drawX >= 1)
                            bComboDraw[drawX - 1][drawY] += COMBO_CLOUD_MARK;
                    } else {
                        ++bComboDraw[drawX + 1][drawY];
                        ++bComboDraw[drawX][drawY + 1];
                        if (drawY >= 1)
                            ++*(bComboDraw[drawX] + drawY - 1);
                        if (drawX >= 1)
                            ++bComboDraw[drawX - 1][drawY];
                    }
                }
            }
        }
    }

    for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++) {
        for (drawY = 0; drawY < ADVMGR_VIEW_CELL_COUNT; drawY++) {
            if (bComboDraw[drawX][drawY]) {
                if (originX + drawX < 0 || originX + drawX >= MAP_CELL_GRID_SIZE
                    || originY + drawY < 0 || originY + drawY >= MAP_CELL_GRID_SIZE)
                    bComboDraw[drawX][drawY] = 0;
                else if (bComboDraw[drawX][drawY] < COMBO_CLOUD_MARK
                         && !GetCloudLookup(drawX + originX, drawY + originY))
                    bComboDraw[drawX][drawY] = 0;
            }
        }
    }

    if (gpMouseManager->IsVis()) {
        drawX = gpMouseManager->m_savedLeft >> CELL_PIXEL_SHIFT;
        drawY = gpMouseManager->m_savedTop >> CELL_PIXEL_SHIFT;
        ++bComboDraw[drawX][drawY];
        ++bComboDraw[drawX + 1][drawY];
        ++bComboDraw[drawX][drawY + 1];
        ++bComboDraw[drawX + 1][drawY + 1];
        ++bComboDraw[drawX + COMBO_FAR_NEIGHBOR_OFFSET][drawY + 1];
    }
    if (m_heroContextLocked) {
        for (drawY = ADVMGR_VIEW_CENTER - 1; drawY <= ADVMGR_VIEW_CENTER + 1; drawY++)
            for (drawX = ADVMGR_VIEW_CENTER - 1; drawX <= ADVMGR_VIEW_CENTER + 1; drawX++)
                ++bComboDraw[drawX][drawY];
    }
    if (m_cursorType == ADVMGR_HERO_ICON_BOAT) {
        ++bComboDraw[ADVMGR_VIEW_CENTER - 1][ADVMGR_VIEW_CENTER - 2];
        ++bComboDraw[ADVMGR_VIEW_CENTER][ADVMGR_VIEW_CENTER - 2];
        ++bComboDraw[ADVMGR_VIEW_CENTER + 1][ADVMGR_VIEW_CENTER - 2];
    }

    for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++) {
        if (bComboDraw[drawX][0])
            DrawCell(
                originX + drawX,
                originY,
                drawX,
                0,
                ADVMGR_DRAW_GROUND | ADVMGR_DRAW_OBJECT,
                0,
                0
            );
    }
    for (drawY = 1; drawY < ADVMGR_VIEW_CELL_COUNT; drawY++) {
        for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++) {
            if (bComboDraw[drawX][drawY])
                DrawCell(originX + drawX, originY + drawY, drawX, drawY, ADVMGR_DRAW_GROUND, 0, 0);
        }
    }
    for (drawY = 1; drawY < ADVMGR_VIEW_CELL_COUNT; drawY++) {
        PollSound();
        for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++) {
            if (bComboDraw[drawX][drawY - 1])
                DrawCell(
                    originX + drawX,
                    originY + drawY - 1,
                    drawX,
                    drawY - 1,
                    ADVMGR_DRAW_OVERLAY | ADVMGR_DRAW_HERO,
                    0,
                    0
                );
        }
        for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++) {
            if (bComboDraw[drawX][drawY])
                DrawCell(originX + drawX, originY + drawY, drawX, drawY, ADVMGR_DRAW_OBJECT, 0, 0);
        }
    }
    for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++) {
        if (bComboDraw[drawX][ADVMGR_VIEW_CELL_COUNT - 1])
            DrawCell(
                originX + drawX,
                originY + ADVMGR_VIEW_CELL_COUNT - 1,
                drawX,
                ADVMGR_VIEW_CELL_COUNT - 1,
                ADVMGR_DRAW_OVERLAY | ADVMGR_DRAW_HERO,
                0,
                0
            );
    }
    for (drawY = 0; drawY < ADVMGR_VIEW_CELL_COUNT; drawY++) {
        for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++) {
            if (bComboDraw[drawX][drawY])
                DrawCell(originX + drawX, originY + drawY, drawX, drawY, ADVMGR_DRAW_CLOUD, 0, 0);
        }
    }

    PollSound();
    UpdBottomView(0, 1, 1);
    DrawAdventureBorder();
    gLimitUpdMinX = ADVMGR_VIEW_CELL_COUNT;
    giLimitUpdMinY = ADVMGR_VIEW_CELL_COUNT;
    giLimitUpdMaxX = 0;
    giLimitUpdMaxY = 0;
    updateCount = 0;
    for (drawY = 0; drawY < ADVMGR_VIEW_CELL_COUNT; drawY++) {
        for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++) {
            if (bComboDraw[drawX][drawY]) {
                updateCount++;
                if (drawX < gLimitUpdMinX)
                    gLimitUpdMinX = drawX;
                if (drawX > giLimitUpdMaxX)
                    giLimitUpdMaxX = drawX;
                if (drawY < giLimitUpdMinY)
                    giLimitUpdMinY = drawY;
                if (drawY > giLimitUpdMaxY)
                    giLimitUpdMaxY = drawY;
            }
        }
    }
    gLimitUpdMinX <<= CELL_PIXEL_SHIFT;
    giLimitUpdMinY <<= CELL_PIXEL_SHIFT;
    giLimitUpdMaxX = ((giLimitUpdMaxX + 1) << CELL_PIXEL_SHIFT) - 1;
    giLimitUpdMaxY = ((giLimitUpdMaxY + 1) << CELL_PIXEL_SHIFT) - 1;
    if (gLimitUpdMinX < BORDER_EDGE_SIZE)
        gLimitUpdMinX = BORDER_EDGE_SIZE;
    if (giLimitUpdMaxX > BORDER_MIDDLE_END - 1)
        giLimitUpdMaxX = BORDER_MIDDLE_END - 1;
    if (giLimitUpdMinY < BORDER_EDGE_SIZE)
        giLimitUpdMinY = BORDER_EDGE_SIZE;
    if (giLimitUpdMaxY > BORDER_MIDDLE_END - 1)
        giLimitUpdMaxY = BORDER_MIDDLE_END - 1;
    if (gLimitUpdMinX > giLimitUpdMaxX || giLimitUpdMinY > giLimitUpdMaxY) {
        gLimitUpdMinX = giLimitUpdMaxX - 1;
        giLimitUpdMinY = giLimitUpdMaxY - 1;
        return 0;
    }
    return 1;
}

// ComboDraw(update) forwards the current map origin.
VA(0x0040f009, 0x2f)
i8 advManager::ComboDraw(i32 update) {
    return ComboDraw(m_mapOriginX, m_mapOriginY, update);
}

VA(0x0040f038, 0x2bb)
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
                m_activeSounds[edgeOffset].volume = ENVIRONMENT_SOUND_DEFAULT_VOLUME;
            } else {
                m_activeSounds[edgeOffset].volume = ENVIRONMENT_SOUND_DEFAULT_VOLUME;
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
                && m_activeSounds[edgeOffset].volume > ENVIRONMENT_SOUND_MAX_DISTANCE) {
                StopSample(m_loopingSamples[m_activeSounds[edgeOffset].soundId]);
                m_activeSounds[edgeOffset].soundId = MAP_SOUND_NONE;
            }
            if (m_activeSounds[edgeOffset].soundId != MAP_SOUND_NONE
                && (m_activeSoundMask & (1 << m_activeSounds[edgeOffset].soundId)) != 0) {
                m_loopingSamples[m_activeSounds[edgeOffset].soundId]->m_playbackData.volume =
                    gEnvironmentVolume[m_activeSounds[edgeOffset].volume];
                UpdateSampleVolume(m_loopingSamples[m_activeSounds[edgeOffset].soundId]);
            }
        }
    }
}

VA(0x0040f2f3, 0x5a)
void advManager::CheckLoadSample(i32 index) {
    if (m_loopingSamples[index] == NULL) {
        TrimLoopingSounds(ADVMGR_ACTIVE_SOUND_COUNT);
        sprintf(gText, "loop%04d.82M", index);
        m_loopingSamples[index] = gpResourceManager->GetSample(gText);
    }
}

VA(0x0040f34d, 0x20f)
void advManager::InsertSound(i16 x, i16 y, i16 distance, i8 soundLayer) {
    i32 slot;
    i32 distanceLimit;
    i32 i;
    i32 soundId;

    if (x < 0 || y < 0 || x >= MAP_CELL_GRID_SIZE || y >= MAP_CELL_GRID_SIZE)
        return;
    soundId = gpGame->m_mapSounds[x][y];
    if (soundId == MAP_SOUND_NONE)
        return;
    for (i = 0; i < ADVMGR_ACTIVE_SOUND_COUNT; ++i) {
        if (m_activeSounds[i].soundId == soundId) {
            if (m_activeSounds[i].volume > distance) {
                m_activeSounds[i].volume = distance;
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
        if (m_activeSounds[i].volume > distanceLimit) {
            distanceLimit = m_activeSounds[i].volume;
            slot = i;
        }
    }
    if (slot != ENVIRONMENT_SOUND_NO_SLOT) {
        if (m_activeSounds[slot].soundId != MAP_SOUND_NONE)
            StopSample(m_loopingSamples[m_activeSounds[slot].soundId]);
        m_activeSounds[slot].soundId = soundId;
        m_activeSounds[slot].volume = distance;
        CheckLoadSample(soundId);
        m_loopingSamples[soundId]->m_playbackData.volume = gEnvironmentVolume[distance];
        m_loopingSamples[soundId]->m_playbackData.repeat = 1;
        PlaySample(m_loopingSamples[soundId]);
        m_activeSoundMask ^= 1 << m_activeSounds[slot].soundId;
    }
}

// ADVMGR .bss keeps four objects no code references: iThisMaxY, iThisMinY,
// USMsg and CDMsg.
i32 iThisMaxY;
i32 iThisMinY;

VA(0x0040f55c, 0x2f7)
void advManager::TeleportTo(i32 x, i32 y, i32) {
    i32 savedShow;
    i32 curFizzle;
    mapCell* location;
    mapCell* savedOldCell;
    i32 hold;
    i8 newTerrain;
    hero* mapHero;
    town* occupiedTown;

    savedShow = bShowIt;
    mapHero = gpGame->GetHero(gpCurPlayer->m_currentHero);
    location = GetCell(x, y);
    savedOldCell = GetCell(m_mapOriginX + ADVMGR_VIEW_CENTER, m_mapOriginY + ADVMGR_VIEW_CENTER);
    if (mapHero->m_locationType == MAP_EVENT_TRIGGER(MAP_OBJECT_TOWN)) {
        occupiedTown = gpGame->GetTown(mapHero->m_occupiedTown);
        occupiedTown->m_occupyingHeroId = TOWN_OCCUPYING_HERO_NONE;
    }
    if (savedOldCell->m_flags & MAP_CELL_HERO_CURSOR)
        savedOldCell->m_flags -= MAP_CELL_HERO_CURSOR;
    CompleteDraw(0);
    if (!gbHumanPlayer[giCurPlayer]) {
        if (!gConfig.blackoutComputer && !gRemoteOn
            && (gpGame->m_mapExtra[mapHero->m_x][mapHero->m_y] & gCurWatchPlayerHighBit))
            bShowIt = 1;
        else
            bShowIt = 0;
    }
    if (savedShow)
        HideRoute(1, 1, 1);
    if (bShowIt) {
        m_mapOriginX = x - ADVMGR_VIEW_CENTER;
        m_mapOriginY = y - ADVMGR_VIEW_CENTER;
        DelayMilli(90);
    }
    mapHero->m_x = x;
    mapHero->m_y = y;
    gpGame->SetVisibility(
        m_mapOriginX + ADVMGR_VIEW_CENTER,
        m_mapOriginY + ADVMGR_VIEW_CENTER,
        giCurPlayer,
        gHeroScoutRadius[mapHero->m_heroClass]
    );
    if (bShowIt) {
        location->m_flags |= MAP_CELL_HERO_CURSOR;
        gpMouseManager->ReallyHidePointer();
        gpWindowManager->SaveFizzleSource(
            BORDER_EDGE_SIZE,
            BORDER_EDGE_SIZE,
            ADVENTURE_VIEWPORT_INNER_SIZE,
            ADVENTURE_VIEWPORT_INNER_SIZE
        );
        CompleteDraw(0);
        PollSound();
        curFizzle = TELEPORT_FIZZLE_TIME;
        if (!gbHumanPlayer[giCurPlayer])
            curFizzle -= TELEPORT_REMOTE_FIZZLE_ADJUSTMENT;
        gpWindowManager->FizzleForward(
            BORDER_EDGE_SIZE,
            BORDER_EDGE_SIZE,
            ADVENTURE_VIEWPORT_INNER_SIZE,
            ADVENTURE_VIEWPORT_INNER_SIZE,
            FIZZLE_USE_DEFAULT_DELAY
        );
        PollSound();
        gpMouseManager->ReallyShowPointer();
    }
    SetEnvironmentOrigin(m_mapOriginX + ADVMGR_VIEW_CENTER, m_mapOriginY + ADVMGR_VIEW_CENTER, 1);
    newTerrain = CELL_TERRAIN(location);
    if (newTerrain != m_currentTerrain) {
        m_currentTerrain = newTerrain;
        PlayMusic(m_currentTerrain);
    }
    Reseed(0, 0);
    UpdateRadar(1, 0);
    CompleteDraw(0);
    ForceNewHover();
}

VA(0x0040f853, 0x1fd)
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
    gpWindowManager->DoDialog(window, DimensionDoorHandler, 0);
    delete window;
    targetHero = gpGame->GetHero(gpCurPlayer->m_currentHero);
    if (gpWindowManager->m_dialogResult == TRAVEL_DIALOG_ACCEPT) {
        newX = m_mapOriginX + m_lastHoverCell;
        newY = m_mapOriginY + m_hoverCellY;
        targetCell = GetCell(newX, newY);
        if ((targetHero->IsEmbarked() && targetCell->m_tileIndex >= MAP_CELL_TILES_PER_TERRAIN)
            || (!targetHero->IsEmbarked()
                && targetCell->m_tileIndex < MAP_CELL_TILES_PER_TERRAIN)) {
            NormalDialog(
                localization::Tr("adventure.dimension_door.failed"),
                NORMAL_DIALOG_TYPE_OK,
                0x61,
                0x91
            );
            UpdateRadar(1, 0);
        } else {
            PlayMusic(MUSIC_TRACK_TELEPORT);
            TeleportTo(newX, newY, 0);
            PlayMusic(m_currentTerrain);
        }
        gpGame->GetHero(gpCurPlayer->m_currentHero)->UseSpell(SPELL_DIMENSION_DOOR);
    } else {
        UpdateRadar(1, 0);
    }
}

struct tag_message USMsg;
struct tag_message CDMsg;

VA(0x0040fa50, 0x240)
void advManager::TownGate(void) {
    hero* targetHero;
    i32 dist;
    i32 selectedTown;
    i32 i;
    i32 nearestDistance;

    nearestDistance = TOWN_PORTAL_DISTANCE_LIMIT;
    selectedTown = -1;
    targetHero = gpGame->GetHero(gpCurPlayer->m_currentHero);
    if (targetHero->IsEmbarked()) {
        NormalDialog(localization::Tr("adventure.town_gate.land_required"), NORMAL_DIALOG_TYPE_OK);
        return;
    }
    for (i = 0; i < gpCurPlayer->m_townCount; i++) {
        dist = MANHATTAN_LENGTH(
            gpGame->m_castleRecs[gpCurPlayer->m_townIds[i]].m_x - targetHero->m_x,
            gpGame->m_castleRecs[gpCurPlayer->m_townIds[i]].m_y - targetHero->m_y
        );
        if (dist < nearestDistance) {
            nearestDistance = dist;
            selectedTown = i;
        }
    }
    if (selectedTown == -1)
        NormalDialog(localization::Tr("adventure.town_gate.no_town"), NORMAL_DIALOG_TYPE_OK);
    if (gpGame->m_castleRecs[gpCurPlayer->m_townIds[selectedTown]].m_occupyingHeroId
        != TOWN_OCCUPYING_HERO_NONE) {
        NormalDialog(localization::Tr("adventure.town_gate.occupied"), NORMAL_DIALOG_TYPE_OK, 0x61);
        return;
    }
    PlayMusic(MUSIC_TRACK_TELEPORT);
    TeleportTo(
        gpGame->m_castleRecs[gpCurPlayer->m_townIds[selectedTown]].m_x,
        gpGame->m_castleRecs[gpCurPlayer->m_townIds[selectedTown]].m_y,
        0
    );
    targetHero->UseSpell(SPELL_TOWN_GATE);
    gpGame->m_castleRecs[gpCurPlayer->m_townIds[selectedTown]].m_occupyingHeroId = targetHero->m_id;
    gpGame->m_castleRecs[gpCurPlayer->m_townIds[selectedTown]].GiveSpells();
    targetHero->m_locationType = MAP_EVENT_TRIGGER(MAP_OBJECT_TOWN);
    targetHero->m_occupiedTown = gpCurPlayer->m_townIds[selectedTown];
    PlayMusic(m_currentTerrain);
}

VA(0x0040fc90, 0x4bb)
void advManager::SummonBoat(void) {
    hero* summonHero;
    mapCell* destinationCell;
    i8 okCell;
    i16 slotIndex;
    i16 iDir;
    i8 foundBoat;
    i8 heroSlot;
    boatRecord* boatRec;
    mapCell* fromCell;
    i16 clipWidth;
    i16 clipX;
    i16 clipY;
    i16 clipHeight;

    summonHero = gpGame->GetHero(gpCurPlayer->m_currentHero);
    okCell = 0;
    foundBoat = 0;
    destinationCell = GetCell(m_mapOriginX + ADVMGR_VIEW_CENTER, m_mapOriginY + ADVMGR_VIEW_CENTER);
    if (destinationCell->m_tileIndex < MAP_CELL_TILES_PER_TERRAIN)
        goto summon_done;
    for (iDir = 0; iDir < MAP_DIRECTION_COUNT; iDir++) {
        destinationCell = GetCell(
            m_mapOriginX + normalDirTable[iDir].x + ADVMGR_VIEW_CENTER,
            m_mapOriginY + normalDirTable[iDir].y + ADVMGR_VIEW_CENTER
        );
        if (destinationCell->m_objectIndex == MAP_CELL_NO_FRAME
            && destinationCell->m_tileIndex < MAP_CELL_TILES_PER_TERRAIN) {
            okCell = 1;
            break;
        }
    }
    if (okCell) {
        heroSlot = gpCurPlayer->CurrentHero();
        for (slotIndex = 0; slotIndex < GAME_BOAT_COUNT; slotIndex++) {
            if (gpGame->m_boatSlots[slotIndex] != GAME_TABLE_FREE
                && gpGame->m_boats[slotIndex].heroId == (heroSlot | BOAT_OCCUPIED_FLAG)) {
                foundBoat = 1;
                break;
            }
        }
        if (!foundBoat) {
            for (slotIndex = 0; slotIndex < GAME_BOAT_COUNT; slotIndex++) {
                if (gpGame->m_boatSlots[slotIndex] != GAME_TABLE_FREE
                    && (gpGame->m_boats[slotIndex].heroId & BOAT_OCCUPIED_FLAG)
                    && gpGame->m_boats[slotIndex].owner == giCurPlayer) {
                    foundBoat = 1;
                    break;
                }
            }
        }
        if (foundBoat) {
            boatRec = &gpGame->m_boats[slotIndex];
            fromCell = GetCell(boatRec->x, boatRec->y);
            gpGame->RestoreCell(
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
                gpWindowManager->SaveFizzleSource(clipX, clipY, clipWidth, clipHeight);
                CompleteDraw(m_mapOriginX, m_mapOriginY, 0);
                gpWindowManager
                    ->FizzleForward(clipX, clipY, clipWidth, clipHeight, FIZZLE_USE_DEFAULT_DELAY);
            }
            boatRec->x = m_mapOriginX + normalDirTable[iDir].x + ADVMGR_VIEW_CENTER;
            boatRec->y = m_mapOriginY + normalDirTable[iDir].y + ADVMGR_VIEW_CENTER;
            boatRec->savedTriggerType = destinationCell->m_triggerType;
            boatRec->savedEventData = destinationCell->m_objectMetadata;
            destinationCell->m_triggerType = MAP_EVENT_TRIGGER(MAP_OBJECT_SHIP);
            destinationCell->m_objectMetadata = slotIndex;
            gpWindowManager->SaveFizzleSource(176, 192, 128, 96);
            CompleteDraw(m_mapOriginX, m_mapOriginY, 0);
            gpWindowManager->FizzleForward(
                SUMMON_TARGET_X,
                SUMMON_TARGET_Y,
                SUMMON_TARGET_WIDTH,
                SUMMON_TARGET_HEIGHT,
                FIZZLE_USE_DEFAULT_DELAY
            );
        }
    }

summon_done:
    UpdateScreen(0, 0);
    Reseed(0, 0);
    if (!foundBoat)
        NormalDialog(
            localization::Tr("adventure.summon_boat.failed"),
            NORMAL_DIALOG_TYPE_OK,
            0x61,
            0x91
        );
}

VA(0x0041014b, 0x2de)
void advManager::ShowRoute(i32 redraw, i32, i32 updateButton) {
    hero* hero;
    i32 reachable;
    i32 fromDir;
    i32 remain;
    i32 mapX;
    i32 index;
    i32 mapY;
    i32 dir;
    i32 terr;
    i16 buttonFrame;

    reachable = 0;
    if (!gbThisNetHumanPlayer[giCurPlayer] && (!giDebugLevel || !gShowComputerRoute))
        return;
    if (gpCurPlayer->m_currentHero == HERO_ID_NONE) {
        HideRoute(redraw, 0, 1);
        return;
    }
    hero = gpGame->GetHero(gpCurPlayer->m_currentHero);
    if (hero->m_destinationX == HERO_DESTINATION_NONE) {
        HideRoute(redraw, 1, 1);
        return;
    }
    gpSearchArray->BuildPath(
        hero->m_x,
        hero->m_y,
        hero->m_destinationX,
        hero->m_destinationY,
        SEARCH_UNLIMITED_COST
    );
    if (gpSearchArray->m_pathLength > 0) {
        memset(m_visibilityMap, 0, MAP_CELL_GRID_SIZE * MAP_CELL_GRID_SIZE);
        m_routeShown = 1;
        remain = hero->m_remainingMobility;
        mapX = hero->m_x;
        mapY = hero->m_y;
        for (index = gpSearchArray->m_pathLength - 1; index >= 0; --index) {
            dir = gpSearchArray->m_directions[index];
            terr = CELL_TERRAIN(GetCell(mapX, mapY));
            remain -=
                CalcTerrainCost(terr, dir & MAP_DIRECTION_DIAGONAL_BIT, remain, hero->m_heroClass);
            mapX += normalDirTable[dir].x;
            mapY += normalDirTable[dir].y;
            if (index == 0) {
                m_visibilityMap[mapX + mapY * MAP_CELL_GRID_SIZE] = ROUTE_CELL_DESTINATION;
            } else {
                fromDir = gpSearchArray->m_directions[index - 1];
                m_visibilityMap[mapX + mapY * MAP_CELL_GRID_SIZE] = gRouteFrame[fromDir][dir];
            }
            if (remain >= 0) {
                m_visibilityMap[mapX + mapY * MAP_CELL_GRID_SIZE] += ROUTE_CELL_REACHABLE_OFFSET;
                reachable = 1;
            }
        }
        if (updateButton) {
            buttonFrame = reachable ? WIDGET_COMMAND_CLEAR_FLAGS : WIDGET_COMMAND_SET_FLAGS;
            gpWindowManager->BroadcastMessage(
                MESSAGE_WIDGET,
                buttonFrame,
                ADVENTURE_CONTROL_CONTINUE_ROUTE,
                WIDGET_FLAG_UPDATE | WIDGET_FLAG_DIMMED
            );
        }
    } else {
        HideRoute(redraw, 1, 1);
    }
    if (redraw) {
        CompleteDraw(0);
        gpMouseManager->ReallyHidePointer();
        UpdateScreen(0, 0);
        gpMouseManager->ReallyShowPointer();
    }
}

VA(0x00410429, 0xd1)
void advManager::HideRoute(i32 redraw, i32 clearDestination, i32 updateButton) {
    hero* currentHero;

    if (!gbThisNetHumanPlayer[giCurPlayer] && (!giDebugLevel || !gShowComputerRoute))
        return;

    if (updateButton)
        gpWindowManager->BroadcastMessage(
            MESSAGE_WIDGET,
            WIDGET_COMMAND_SET_FLAGS,
            PANEL_CONTINUE_ROUTE,
            WIDGET_FLAG_UPDATE | WIDGET_FLAG_DIMMED
        );

    if (clearDestination && gpCurPlayer->m_currentHero != HERO_ID_NONE) {
        currentHero = gpGame->GetHero(gpCurPlayer->m_currentHero);
        currentHero->m_destinationX = HERO_DESTINATION_NONE;
        currentHero->m_destinationY = HERO_DESTINATION_NONE;
    }

    if (!m_routeShown)
        return;

    m_routeShown = 0;
    if (redraw) {
        CompleteDraw(0);
        UpdateScreen(0, 0);
    }
}

VA(0x004104fa, 0x7d)
void advManager::CheckDimHero(void) {
    if (!gbThisNetHumanPlayer[giCurPlayer] || gpCurPlayer->CurrentHero() == HERO_ID_NONE)
        return;
    if (!gpGame->IsMobile(gpCurPlayer->CurrentHero())) {
        ShowRoute(1, 0, 0);
        UpdateHeroLocators(1, 1);
        gpAdvManager->CheckDimNextHeroBut();
    }
}

VA(0x00410577, 0x63)
void advManager::CheckDimNextHeroBut(void) {
    i16 frame;

    frame = gbThisNetHumanPlayer[giCurPlayer] && gpCurPlayer->HasMobileHero()
                ? static_cast<i16>(WIDGET_COMMAND_CLEAR_FLAGS)
                : static_cast<i16>(WIDGET_COMMAND_SET_FLAGS);
    gpWindowManager->BroadcastMessage(
        MESSAGE_WIDGET,
        frame,
        BUTTON_BROADCAST_ARG,
        WIDGET_FLAG_UPDATE | WIDGET_FLAG_DIMMED
    );
}

VA(0x004105da, 0x12d)
void advManager::SeedTo(i32 targetX, i32 targetY) {
    hero* currentHero;

    if (!gbThisNetHumanPlayer[giCurPlayer])
        return;
    if (gpCurPlayer->m_currentHero == HERO_ID_NONE)
        return;

    currentHero = gpGame->GetHero(gpCurPlayer->m_currentHero);
    if (!giSeedingValid)
        gpSearchArray->SeedPosition(
            currentHero->m_x,
            currentHero->m_y,
            m_cursorDirection,
            SEARCH_UNLIMITED_COST,
            m_cursorType == ADVMGR_HERO_ICON_BOAT,
            0,
            currentHero->m_remainingMobility,
            currentHero->m_heroClass,
            targetX,
            targetY,
            0,
            1
        );
    else if (!gFullySeeded)
        gpSearchArray->SeedPosition(
            currentHero->m_x,
            currentHero->m_y,
            m_cursorDirection,
            SEARCH_UNLIMITED_COST,
            m_cursorType == ADVMGR_HERO_ICON_BOAT,
            0,
            currentHero->m_remainingMobility,
            currentHero->m_heroClass,
            targetX,
            targetY,
            1,
            1
        );
}

// Routes the hover through a message record.
VA(0x00410707, 0x3f)
void advManager::ForceNewHover(void) {
    struct tag_message msg;

    if (!gbThisNetHumanPlayer[giCurPlayer])
        return;
    m_lastHoverCell = CURSOR_INVALID_POSITION;
    msg.id = ADVENTURE_CONTROL_MAP_VIEW;
    ProcessHover(&msg);
}

VA(0x00410746, 0x1d0)
void advManager::ScreenScroll(i8 direction, i32 updatePointer) {
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
        gpMouseManager->SetPointer(direction + HOVER_SCROLL_FRAME_FIRST);

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
        UpdateRadar(1, 0);
        CompleteDraw(0);
        UpdateScreen(0, 0);
    }
}

VA(0x00410916, 0x197)
void advManager::CheckScreenScroll(void) {
    i16 mouseX;
    i16 mouseY;
    i32 oldMapX;
    i32 oldMapY;

    if (KBTickCount() - gLastScrollTime > SCROLL_TICK_INTERVAL) {
        gLastScrollTime = KBTickCount();
        oldMapX = m_mapOriginX;
        oldMapY = m_mapOriginY;
        gpMouseManager->MouseCoords(mouseX, mouseY);

        if (mouseX >= 0 && mouseX < LOGICAL_SCREEN_WIDTH && mouseY >= 0
            && mouseY < LOGICAL_SCREEN_HEIGHT) {
            if (mouseX < SCROLL_BORDER) {
                if (mouseY < SCROLL_BORDER)
                    ScreenScroll(MAP_DIRECTION_NORTH_WEST, 1);
                else if (mouseY > LOGICAL_SCREEN_HEIGHT - SCROLL_BORDER)
                    ScreenScroll(MAP_DIRECTION_SOUTH_WEST, 1);
                else
                    ScreenScroll(MAP_DIRECTION_WEST, 1);
            } else if (mouseX > LOGICAL_SCREEN_WIDTH - SCROLL_BORDER - 1) {
                if (mouseY < SCROLL_BORDER)
                    ScreenScroll(MAP_DIRECTION_NORTH_EAST, 1);
                else if (mouseY > LOGICAL_SCREEN_HEIGHT - SCROLL_BORDER)
                    ScreenScroll(MAP_DIRECTION_SOUTH_EAST, 1);
                else
                    ScreenScroll(MAP_DIRECTION_EAST, 1);
            } else if (mouseY < SCROLL_BORDER) {
                ScreenScroll(MAP_DIRECTION_NORTH, 1);
            } else if (mouseY > LOGICAL_SCREEN_HEIGHT - SCROLL_BORDER) {
                ScreenScroll(MAP_DIRECTION_SOUTH, 1);
            }
        }

        if (gpMouseManager->m_cursorFrame >= HOVER_SCROLL_FRAME_FIRST
            && gpMouseManager->m_cursorFrame < HOVER_SCROLL_FRAME_END && oldMapX == m_mapOriginX
            && oldMapY == m_mapOriginY)
            gpMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
    }
}

VA(0x00410aad, 0x79)
i32 advManager::MouseInScrollZone(void) {
    i16 mouseX;
    i16 mouseY;

    gpMouseManager->MouseCoords(mouseX, mouseY);
    if (mouseX >= 0 && mouseX < LOGICAL_SCREEN_WIDTH && mouseY >= 0
        && mouseY < LOGICAL_SCREEN_HEIGHT) {
        if (mouseX < SCROLL_BORDER || mouseX > LOGICAL_SCREEN_WIDTH - SCROLL_BORDER - 1
            || mouseY < SCROLL_BORDER || mouseY > LOGICAL_SCREEN_HEIGHT - SCROLL_BORDER) {
            return 1;
        }
    }
    return 0;
}

VA(0x00410b26, 0x248)
void advManager::SetInitialMapOrigin(void) {
    i16 x;
    i16 y;
    game* gameStateItem;
    hero* heroPtr;
    town* townPointer;
    town* ownTownNode;

    gpWindowManager->BroadcastMessage(
        MESSAGE_WIDGET,
        WIDGET_COMMAND_SET_FLAGS,
        ADVENTURE_CONTROL_CONTINUE_ROUTE,
        WIDGET_FLAG_UPDATE | WIDGET_FLAG_DIMMED
    );
    m_lastHoverCell = m_hoverCellY = 0;
    m_cursorActive = 0;
    gHeroMoving = 0;
    if (gpCurPlayer->CurrentTown() != GAME_TOWN_NONE) {
        townPointer = gpGame->GetTown(gpCurPlayer->m_currentTown);
        m_mapOriginX = townPointer->m_x - ADVMGR_VIEW_CENTER;
        m_mapOriginY = townPointer->m_y - ADVMGR_VIEW_CENTER;
    } else if (gpCurPlayer->CurrentHero() != HERO_ID_NONE) {
        MobilizeCurrHero(0);
    } else if (gpCurPlayer->m_heroCount > 0) {
        heroPtr = &gpGame->m_heroRecs[gpCurPlayer->m_heroIds[0]];
        m_mapOriginX = heroPtr->m_x - ADVMGR_VIEW_CENTER;
        m_mapOriginY = heroPtr->m_y - ADVMGR_VIEW_CENTER;
    } else if (gpCurPlayer->m_townCount > 0) {
        ownTownNode = &gpGame->m_castleRecs[gpCurPlayer->m_townIds[0]];
        m_mapOriginX = ownTownNode->m_x - ADVMGR_VIEW_CENTER;
        m_mapOriginY = ownTownNode->m_y - ADVMGR_VIEW_CENTER;
    } else {
        m_mapOriginX = 0;
        m_mapOriginY = 0;
    }
    m_currentTerrain = giGroundToTerrain
        [GetCell(m_mapOriginX + ADVMGR_VIEW_CENTER, m_mapOriginY + ADVMGR_VIEW_CENTER)
             ->m_tileIndex];
    PlayMusic(m_currentTerrain);
    SetEnvironmentOrigin(m_mapOriginX + ADVMGR_VIEW_CENTER, m_mapOriginY + ADVMGR_VIEW_CENTER, 1);
    gpMouseManager->MouseCoords(x, y);
    gpMouseManager->WarpPointer(x - 20, y - 20);
    Reseed(0, 0);
    CheckDimNextHeroBut();
}

VA(0x00410d6e, 0x131)
void advManager::LoadRemote(void) {
    gpMouseManager->ReallyHidePointer();
    if (gbThisNetHumanPlayer[giCurPlayer])
        gpMouseManager->SetPointer("advmice.mse", ADVENTURE_POINTER_DEFAULT);
    gpGame->LoadGame("REMOTE.GAM", 0, 1);
    if (gbThisNetHumanPlayer[giCurPlayer])
        gpGame->CancelComputerScreen();
    gpGame->DoNewTurn();
    UpdateHeroLocators(1, 1);
    UpdateTownLocators(1, 1);
    UpdateRadar(1, 0);
    gpMouseManager->ReallyShowPointer();
    UpdBottomView(1, 1, 1);
    if ((gpGame->m_day != 1 || (gpGame->m_week == 1 && gpGame->m_month == 1)) && gRemoteOn
        && gbThisNetHumanPlayer[giCurPlayer] && gForceSwitchMusic == FORCED_MUSIC_IDLE) {
        PlayMusic(MUSIC_TRACK_NETWORK_TURN);
        gForceSwitchMusic = KBTickCount();
    }
    gpAdvManager->ForceNewHover();
}

VA(0x00410e9f, 0x13b)
char* advManager::CheckHandleNet(void) {
    RemoteMessage* receivedPacket;
    i32 exitedFlag;

    // API-forced: GetRemoteData and DoNetCombat pass queue records as char*.
    receivedPacket = reinterpret_cast<RemoteMessage*>(GetRemoteData(1));
    if (receivedPacket && receivedPacket->type == REMOTE_MESSAGE_RELIABLE) {
        switch (receivedPacket->command) {
            case BOX_REMOTE_SAVE:
                exitedFlag = receivedPacket->payload.playerExited;
                if (!gpGame->ReceiveSaveGame(
                        receivedPacket->payload.saveSize,
                        receivedPacket->sender
                    ))
                    ShutDown(NULL);
                if (exitedFlag)
                    ReceiveRemotePlayerExit(receivedPacket->sender, 0, 1, 0);
                LoadRemote();
                break;
            case REMOTE_COMMAND_CHAT:
                PopNetBox(receivedPacket->payload.data);
                break;
            case REMOTE_COMMAND_HERO_TOWN_DATA:
                if (gInCombat)
                    return reinterpret_cast<char*>(receivedPacket); // API-forced: char* record.
                else
                    DoNetCombat(
                        reinterpret_cast<char*>(receivedPacket)
                    ); // API-forced: char* record.
                break;
            case REMOTE_COMMAND_PLAYER_EXIT:
                ReceiveRemotePlayerExit(
                    receivedPacket->payload.data[0],
                    receivedPacket->payload.data[1],
                    0,
                    0
                );
                break;
            default:
                return reinterpret_cast<char*>(receivedPacket); // API-forced: char* record.
        }
    }
    return NULL;
}

VA(0x00410fda, 0xa2)
H1_ENUM_RETURN(MessageDispatchResult, i16) advManager::CheckHandleNetPlayerWait(struct tag_message& message, i8 doMain) {
    if (message.type == MESSAGE_MOUSE_MOVE)
        gpMouseManager->Main(message);

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

    UpdBottomView(0, 1, 1);
    return MESSAGE_DISPATCH_CONTINUE;
}

VA(0x0041107c, 0x1a2)
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
            gpResourceManager->Dispose(m_loopingSamples[i]);
            m_loopingSamples[i] = NULL;
        }
    }
}

VA(0x0041121e, 0xc3)
void advManager::DisableButtons(void) {
    if (gpAdvManager->m_active != 1)
        return;
    struct tag_message message;
    SET_ADVENTURE_BUTTON_FLAGS(message, m_adventureWindow, WIDGET_COMMAND_CLEAR_FLAGS);
}

VA(0x004112e1, 0xc3)
void advManager::EnableButtons(void) {
    if (gpAdvManager->m_active != 1)
        return;
    struct tag_message message;
    SET_ADVENTURE_BUTTON_FLAGS(message, m_adventureWindow, WIDGET_COMMAND_SET_FLAGS);
}

VA(0x004113a4, 0x13e)
void advManager::SaveAdventureBorder(void) {
    if (m_adventureBorder != NULL)
        return;

    m_adventureBorder = static_cast<u8*>(malloc(BORDER_BUFFER_SIZE));
    u8* savedPixels = m_adventureBorder;
    u8* screen = gpWindowManager->m_screen->m_pixels;
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

VA(0x004114e2, 0x134)
void advManager::DrawAdventureBorder(void) {
    u8* savedPixels;
    u8* screen;
    i32 row;

    if (m_adventureBorder == NULL)
        return;
    if (gNoBorder != 0)
        return;

    screen = gpWindowManager->m_screen->m_pixels;
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

// ADVMGR globals. Retail emits some among the literals of their users.
DATA(0x0048e140)
i32 gLimitUpdMinX = UPDATE_NONE;
DATA(0x004a673c)
i32 gLastScrollTime = 0;
DATA(0x004a6740)
i32 gSandAnim = 0;
DATA(0x004a6744)
i32 gLastHourGlassUpdateTime = 0;
DATA(0x004a6748)
i32 TrigX = 0;
DATA(0x004a674c)
i32 TrigY = 0;
DATA(0x004a6750)
i32 gCurBottomView = BOTTOM_VIEW_NONE;
DATA(0x0048e144)
i32 gCurBottomViewEnemy = BOTTOM_VIEW_NO_ENEMY;
DATA(0x004a6754)
i32 gCurHourGlassPhase = 0;
DATA(0x0048e148)
i32 gLastHourGlassPhase = 1;
// No retail code reads this value.
DATA(0x0048e14c)
i32 gUnusedAdventureValue = 28;
DATA(0x004a6758)
i32 gForceUpdate = 0;
DATA(0x004a6728)
class heroWindow* gPanel;
DATA(0x004a65ac)
i32 giFrameStep;
DATA(0x004a6730)
char cArmySizeName[12];
DATA(0x004a65dc)
i32 giLimitUpdMaxX;
DATA(0x004a65e0)
i32 giLimitUpdMaxY;
DATA(0x004a65e4)
i8 bPrefsChanged;
DATA(0x004a6710)
i32 giLimitUpdMinY;
DATA(0x004a65ec)
i8 bComboDraw[17][17];
DATA(0x004a65cc)
i8 gFreshSave;
DATA(0x004a65a8)
i32 iLastAnimFrame;
// ADVMGR's ambient-sound volume by distance, on a 0..127 scale. Eight
// slots, five initialized: the zero tail is 0x0048a380..0x0048a38b, before
// advManager's vtable.
DATA(0x0048a36c)
const i32 gEnvironmentVolume[8] = {127, 96, 63, 31, 21};

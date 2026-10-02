// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <BASE/BITS.h>
#include <BASE/INPUTMGR_TYPES.h>
#include <BASE/Icon2b.h>
#include <BASE/TILE.h>
#include <BASE/Iconm2b.h>
#include <BASE/Icond2b.h>
#include <BASE/MISC_TYPES.h>
#include <BASE/WINMGR_TYPES.h>
#include <BASE/Misc.h>
#include <BASE/bmap2.h>
#include <H1/All.h>
#include <H1/KB.h>
#include <SOURCE/highScoreRuntime.h>
#include <SOURCE/mapObjectTypes.h>
#include <SOURCE/NOOPT.h>
#include <SOURCE/fileRequester.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/REMOTE.h>
#include <SOURCE/X_GLOBAL.h>
#include <SOURCE/wingraph.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern signed char giCampaignChoice;
extern char gLastFilename[];

// Buka's giSeedingValid is the dword zeroed by retail Reseed at VA 0x4c5170.
// Code-use identity only; no initializer-byte coverage is asserted.
DATA(0x004c5170)
int giSeedingValid;


// clang-format off
H1_ENUM_CONST_BEGIN(AdventureButtonConstant)
    BUTTON_BROADCAST_ARG = 1,
    PANEL_CONTINUE_ROUTE = 2
H1_ENUM_CONST_END(AdventureButtonConstant)

H1_ENUM_CONST_BEGIN(AdventureScreenConstant)
    LOGICAL_SCREEN_WIDTH = 640,
    LOGICAL_SCREEN_HEIGHT = 480,
    SCROLL_BORDER = 16
H1_ENUM_CONST_END(AdventureScreenConstant)

H1_ENUM_CONST_BEGIN(AdventureBorderConstant)
    ADVENTURE_VIEWPORT_EXTENT = 480,
    BORDER_EDGE_SIZE = 16,
    BORDER_SIDE_BYTES = 16,
    BORDER_SAVED_SIDE_BYTES = 32,
    BORDER_MIDDLE_END = 464,
    BORDER_BUFFER_SIZE = 0x7400
H1_ENUM_CONST_END(AdventureBorderConstant)

H1_ENUM_CONST_BEGIN(AdventureLocatorConstant)
    LOCATOR_VISIBLE_COUNT = 4,
    LOCATOR_PAGE_THRESHOLD = 5,
    LOCATOR_PAGE_DENOMINATOR_OFFSET = 4,
    LOCATOR_SCROLL_NO_PAGES_Y = 232
H1_ENUM_CONST_END(AdventureLocatorConstant)

H1_ENUM_BEGIN(BottomViewMode)
    BOTTOM_VIEW_NONE = 0,
    BOTTOM_VIEW_NEW_TURN = 1,
    BOTTOM_VIEW_KINGDOM = 2,
    BOTTOM_VIEW_HERO = 3,
    BOTTOM_VIEW_ENEMY_TURN = 4,
    BOTTOM_VIEW_RESOURCE = 5,
    BOTTOM_VIEW_OVERRIDE_DISABLED = 6
H1_ENUM_END(BottomViewMode)

H1_ENUM_CONST_BEGIN(BottomViewPanelConstant)
    BOTTOM_VIEW_DRAW_FIRST_WIDGET = 2000,
    BOTTOM_VIEW_DRAW_LAST_WIDGET = 2200,
    BOTTOM_VIEW_PANEL_X = 480,
    BOTTOM_VIEW_PANEL_Y = 392,
    BOTTOM_VIEW_PANEL_WIDTH = 143,
    BOTTOM_VIEW_PANEL_HEIGHT = 71
H1_ENUM_CONST_END(BottomViewPanelConstant)

H1_ENUM_CONST_BEGIN(AdventureScrollConstant)
    SCROLL_MIN_ORIGIN = -7,
    SCROLL_MAX_ORIGIN = 64,
    SCROLL_TICK_INTERVAL = 70,
    HOVER_SCROLL_FRAME_FIRST = 32,
    HOVER_SCROLL_FRAME_END = 40
H1_ENUM_CONST_END(AdventureScrollConstant)

H1_ENUM_CONST_BEGIN(AdventurePanelDialogConstant)
    PANEL_CLOSE_WIDGET = 0x7800,
    PANEL_NO_HELP = -1,
    PANEL_VIEW_WORLD_HELP = 0,
    PANEL_VIEW_PUZZLE_HELP = 1,
    PANEL_CAST_SPELL_HELP = 2,
    PANEL_SEARCH_HELP = 3,
    PANEL_VIEW_WORLD = 1,
    PANEL_VIEW_PUZZLE = 2,
    PANEL_CAST_SPELL = 3,
    PANEL_SEARCH = 4
H1_ENUM_CONST_END(AdventurePanelDialogConstant)

// CastSpell charges Dimension Door and Town Gate this much mobility.
H1_ENUM_CONST_BEGIN(AdventureSpellConstant)
    SPELL_TRAVEL_MOBILITY_COST = 12
H1_ENUM_CONST_END(AdventureSpellConstant)

H1_ENUM_BEGIN(AdventureDrawMask)
    ADVMGR_DRAW_GROUND = 0x01,
    ADVMGR_DRAW_OBJECT = 0x02,
    ADVMGR_DRAW_OVERLAY = 0x04,
    ADVMGR_DRAW_HERO = 0x08,
    ADVMGR_DRAW_CLOUD = 0x20,
    ADVMGR_VIEW_CELL_COUNT = 15
H1_ENUM_END(AdventureDrawMask)

H1_ENUM_CONST_BEGIN(AdventurePanelButtonConstant)
    ADVMGR_PANEL_BUTTON_FIRST = 1,
    ADVMGR_PANEL_BUTTON_LAST = 6
H1_ENUM_CONST_END(AdventurePanelButtonConstant)
// clang-format on

// Buka 2.1's unconditional six-button enable/disable broadcast.
#define SET_ADVENTURE_BUTTON_FLAGS(message, window, cmd)                                           \
    ((message).type = MESSAGE_WIDGET,                                                              \
     (message).command = (cmd),                                                     \
     (message).value = WIDGET_FLAG_ENABLED,                                    \
     (message).id = ADVMGR_PANEL_BUTTON_FIRST,                                      \
     (window)->BroadcastMessage(message),                                                          \
     (message).id = ADVMGR_PANEL_BUTTON_FIRST + 1,                                  \
     (window)->BroadcastMessage(message),                                                          \
     (message).id = ADVMGR_PANEL_BUTTON_FIRST + 2,                                  \
     (window)->BroadcastMessage(message),                                                          \
     (message).id = ADVMGR_PANEL_BUTTON_FIRST + 3,                                  \
     (window)->BroadcastMessage(message),                                                          \
     (message).id = ADVMGR_PANEL_BUTTON_FIRST + 4,                                  \
     (window)->BroadcastMessage(message),                                                          \
     (message).id = ADVMGR_PANEL_BUTTON_LAST,                                       \
     (window)->BroadcastMessage(message))

 // donor PoL RVA 0x00056350; preferred Buka symbol ??0advManager@@QAE@XZ
 // donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
 // evidence: graph:2;base=0.538995;margin=0.239070;shape=0.344;size=0.950;calls=1.000;alternate=pol20:void advManager::constructor(void)@0x00056350
 VA(0x004252c0, 0x2cc)
advManager::advManager(void) {
    int i;

    m_groundTiles = NULL;
    m_puzzleIcon = NULL;
    m_mapOriginX = 0;
    m_mapOriginY = 0;
    m_updateMinX = 0;
    m_updateMinY = 0;
    m_updateMaxX = 0;
    m_updateMaxY = 0;
    m_selectedCell = -1;
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
    m_lastQuickViewX = -99;
    m_lastQuickViewY = -99;
    m_animationPhases[0] = 0;
    m_animationPhases[1] = 1;
    m_animationPhases[2] = 3;
    m_animationPhases[3] = 5;
    m_mapData = gpGame->GetWorldMapData();
    gMapX = 0;
    gMapY = 0;
    m_cursorFrameCount = 0;
    m_cursorCycle = 0;
    m_cursorTurning = 0;
}

// InitMainClasses deletes gpAdvManager through this vtable-reset destructor.
VA(0x0042558c, 0x1f)
advManager::~advManager() {}

// donor PoL RVA 0x0005665f; preferred Buka symbol ?Open@advManager@@UAEHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.608500;margin=0.280356;shape=0.449;size=0.637;calls=0.611;strings=advManager|adv_wind.bin|advmice.mse;alternate=pol20:int advManager::Open(int);   // virtual [override (implements baseManager pure virtual)]@0x0005665f
VA(0x004255ab, 0xea6)
short advManager::Open(short id) {
    int savedShowIt;
    int firstTime;
    int oldPlayer;
    int oldVolume;
    int i;

    firstTime = 1;
    iCurBottomView = BOTTOM_VIEW_NONE;
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
        m_scrollLeftButton = new iconWidget(540, 195, 8, 17, "scroll.icn", 4, 0, 26, 16, 1);
        if (m_scrollLeftButton == NULL)
            MemError();
        m_adventureWindow->AddWidget(m_scrollLeftButton, -1);
        m_scrollRightButton = new iconWidget(612, 195, 8, 17, "scroll.icn", 4, 0, 27, 16, 1);
        if (m_scrollRightButton == NULL)
            MemError();
        m_adventureWindow->AddWidget(m_scrollRightButton, -1);
        m_panelBackdrops[0] = new backdropWidget(480, 176, 56, 128, -1, 32);
        if (m_panelBackdrops[0] == NULL)
            MemError();
        m_panelBackdrops[1] = new backdropWidget(552, 176, 56, 128, -1, 32);
        if (m_panelBackdrops[1] == NULL)
            MemError();
        m_panelBackdrops[2] = new backdropWidget(539, 194, 10, 92, -1, 32);
        if (m_panelBackdrops[2] == NULL)
            MemError();
        m_panelBackdrops[3] = new backdropWidget(611, 194, 10, 92, -1, 32);
        if (m_panelBackdrops[3] == NULL)
            MemError();
        m_panelBackdrops[4] = new backdropWidget(480, 320, 144, 144, -1, 32);
        if (m_panelBackdrops[4] == NULL)
            MemError();
        for (i = 0; i < ADVMGR_PANEL_ICON_COUNT; i++)
            m_adventureWindow->AddWidget(m_panelBackdrops[i], -1);
    }
    if (gbThisNetHumanPlayer[giCurPlayer])
        gpMouseManager->SetPointer("advmice.mse", 0);
    else
        gpMouseManager->SetPointer("advmice.mse", 1);
    if (m_visibilityMap == NULL) {
        m_visibilityMap = new signed char[MAP_CELL_GRID_SIZE * MAP_CELL_GRID_SIZE];
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
    if (m_objectIcons[0] == NULL)
        m_objectIcons[0] = gpResourceManager->GetIcon("obj32-00.icn");
    if (m_objectIcons[1] == NULL)
        m_objectIcons[1] = gpResourceManager->GetIcon("obj32-01.icn");
    if (m_objectIcons[2] == NULL)
        m_objectIcons[2] = gpResourceManager->GetIcon("obj32-02.icn");
    if (m_objectIcons[3] == NULL)
        m_objectIcons[3] = gpResourceManager->GetIcon("obj32-03.icn");
    if (m_objectIcons[4] == NULL)
        m_objectIcons[4] = gpResourceManager->GetIcon("obj32-04.icn");
    if (m_objectIcons[5] == NULL)
        m_objectIcons[5] = gpResourceManager->GetIcon("obj32-05.icn");
    if (m_objectIcons[6] == NULL)
        m_objectIcons[6] = gpResourceManager->GetIcon("obj32-06.icn");
    if (m_objectIcons[7] == NULL)
        m_objectIcons[7] = gpResourceManager->GetIcon("obj32-07.icn");
    if (m_objectIcons[8] == NULL)
        m_objectIcons[8] = gpResourceManager->GetIcon("mtn32.icn");
    if (m_objectIcons[9] == NULL)
        m_objectIcons[9] = gpResourceManager->GetIcon("tree32.icn");
    if (m_objectIcons[10] == NULL)
        m_objectIcons[10] = gpResourceManager->GetIcon("town32.icn");
    if (m_objectIcons[11] == NULL)
        m_objectIcons[11] = gpResourceManager->GetIcon("rsrc32.icn");
    if (m_objectIcons[12] == NULL)
        m_objectIcons[12] = gpResourceManager->GetIcon("mons32.icn");
    if (m_objectIcons[13] == NULL)
        m_objectIcons[13] = gpResourceManager->GetIcon("art32.icn");
    if (m_objectIcons[14] == NULL)
        m_objectIcons[14] = gpResourceManager->GetIcon("flag32.icn");
    if (m_objectIcons[15] == NULL)
        m_objectIcons[15] = gpResourceManager->GetIcon("ressmall.icn");
    if (m_objectIcons[16] == NULL)
        m_objectIcons[16] = gpResourceManager->GetIcon("hourglas.icn");
    if (m_objectIcons[17] == NULL)
        m_objectIcons[17] = gpResourceManager->GetIcon("route.icn");
    if (m_objectIcons[18] == NULL)
        m_objectIcons[18] = gpResourceManager->GetIcon("smcrest.icn");
    if (m_objectIcons[19] == NULL)
        m_objectIcons[19] = gpResourceManager->GetIcon("stonback.icn");
    if (m_objectIcons[20] == NULL)
        m_objectIcons[20] = gpResourceManager->GetIcon("minimon.icn");
    if (m_heroIcons[0] == NULL)
        m_heroIcons[0] = gpResourceManager->GetIcon("kngt32.icn");
    if (m_heroIcons[1] == NULL)
        m_heroIcons[1] = gpResourceManager->GetIcon("barb32.icn");
    if (m_heroIcons[2] == NULL)
        m_heroIcons[2] = gpResourceManager->GetIcon("sorc32.icn");
    if (m_heroIcons[3] == NULL)
        m_heroIcons[3] = gpResourceManager->GetIcon("wrlk32.icn");
    if (m_heroIcons[4] == NULL)
        m_heroIcons[4] = gpResourceManager->GetIcon("boat32.icn");
    gbLoadingMonoIcon = 1;
    if (m_boatShadowIcon == NULL)
        m_boatShadowIcon = gpResourceManager->GetIcon("shadow32.icn");
    gbLoadingMonoIcon = 0;
    if (m_flagIcons[0] == NULL)
        m_flagIcons[0] = gpResourceManager->GetIcon("b-flag32.icn");
    if (m_flagIcons[1] == NULL)
        m_flagIcons[1] = gpResourceManager->GetIcon("g-flag32.icn");
    if (m_flagIcons[2] == NULL)
        m_flagIcons[2] = gpResourceManager->GetIcon("r-flag32.icn");
    if (m_flagIcons[3] == NULL)
        m_flagIcons[3] = gpResourceManager->GetIcon("y-flag32.icn");
    if (m_boatFlagIcons[0] == NULL)
        m_boatFlagIcons[0] = gpResourceManager->GetIcon("b-bflg32.icn");
    if (m_boatFlagIcons[1] == NULL)
        m_boatFlagIcons[1] = gpResourceManager->GetIcon("g-bflg32.icn");
    if (m_boatFlagIcons[2] == NULL)
        m_boatFlagIcons[2] = gpResourceManager->GetIcon("r-bflg32.icn");
    if (m_boatFlagIcons[3] == NULL)
        m_boatFlagIcons[3] = gpResourceManager->GetIcon("y-bflg32.icn");
    gbLoadingMonoIcon = 1;
    if (m_puzzleIcon == NULL)
        m_puzzleIcon = gpResourceManager->GetIcon("radar.icn");
    gbLoadingMonoIcon = 0;
    for (i = 0; i < ADVMGR_ENVIRONMENT_SOUND_COUNT; i++)
        m_loopingSamples[i] = NULL;
    for (i = 0; i < ADVMGR_ACTIVE_SOUND_COUNT; i++) {
        m_activeSounds[i].soundId = -1;
        m_activeSounds[i].volume = 127;
        m_activeSoundMask = 0;
    }
    GetCursorSampleSet(gConfig.walkSpeed);
    if (!gbThisNetHumanPlayer[giCurPlayer]) {
        gpGame->TurnOnAIMusic();
        SetNoDialogMenus(0);
    } else {
        SetNoDialogMenus(1);
    }
    glTimers[0] = KBTickCount() + 120;
    m_messageTypeMask = 815;
    gpMouseManager->NewUpdate(1);
    oldVolume = gConfig.soundVolume;
    if (gConfig.soundVolume != 0)
        gConfig.soundVolume = 10;
    SetInitialMapOrigin();
    bShowIt = gbThisNetHumanPlayer[giCurPlayer];
    gpMouseManager->SetColorMice(0);
    oldPlayer = giCurPlayer;
    savedShowIt = bShowIt;
    giCurPlayer = giCurWatchPlayer;
    gpCurPlayer = &gpGame->m_players[giCurPlayer];
    bShowIt = 1;
    RedrawAdvScreen(1);
    giCurPlayer = oldPlayer;
    bShowIt = savedShowIt;
    gpCurPlayer = &gpGame->m_players[giCurPlayer];
    if (!gbThisNetHumanPlayer[giCurPlayer])
        gpGame->ShowComputerScreen();
    gpMouseManager->ReallyShowPointer();
    KBChangeMenu(hmnuAdv);
    gpWindowManager->FadeScreen(0, 8, gPalette);
    giBottomViewOverride = 0;
    gConfig.soundVolume = oldVolume;
    gpSoundManager->AdjustSoundVolumes();
    m_messageMask = 0x400;
    m_priority = id;
    m_active = 1;
    strcpy(m_name, "advManager");
    return 0;
}

// donor PoL RVA 0x00057028; preferred Buka symbol ?Close@advManager@@UAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.555933;margin=0.510723;shape=0.387;size=0.995;calls=0.909;alternate=pol20:void advManager::Close(void);   // virtual [override (implements baseManager pure virtual)]@0x00057028
VA(0x00426451, 0x3bd)
void advManager::Close(void) {
    short index;

    ClearBottomView();
    gpMouseManager->SetPointer(-1);
    gpSoundManager->SwitchAmbientMusic(-1);
    gpSoundManager->StopAllSamples();
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
        gpResourceManager->Dispose(m_cursorSamples[index]);
        m_cursorSamples[index] = NULL;
    }
    gpWindowManager->RemoveWindow(m_adventureWindow);
    delete m_adventureWindow;
    m_adventureWindow = NULL;
    if (m_visibilityMap)
        delete m_visibilityMap;
    m_visibilityMap = NULL;
    iCurBottomView = BOTTOM_VIEW_NONE;
    m_active = 0;
}

// donor PoL RVA 0x00057432; preferred Buka symbol ?GetCursorSampleSet@advManager@@QAEXH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.701218;margin=0.695021;shape=0.378;size=0.921;calls=1.000;strings=wsnd%1d%1d.82M;alternate=pol20:void advManager::GetCursorSampleSet(int)@0x00057432
VA(0x0042680e, 0xc7)
void advManager::GetCursorSampleSet(int sampleSet) {
    if (sampleSet >= 1)
        sampleSet = 2;
    signed char suffixSample[ADVMGR_CURSOR_SAMPLE_COUNT] = {0, 3, 5, 3, 4, 5, 6};
    for (int index = 0; index < ADVMGR_CURSOR_SAMPLE_COUNT; ++index) {
        sprintf(gText, "wsnd%1d%1d.82M", sampleSet, suffixSample[index]);
        m_cursorSamples[index] = gpResourceManager->GetSample(gText);
        m_cursorSamples[index]->m_playbackData.volume = 0x40;
        m_cursorSamples[index]->m_playbackData.channelType = SAMPLE_PLAYBACK_CHANNEL_GROUP;
    }
}

// donor PoL RVA 0x0005751b; preferred Buka symbol ?DoAdvCommand@advManager@@QAEPAVmapCell@@XZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:7;base=0.528946;margin=1.360342;shape=0.339;size=0.921;calls=0.947;alternate=pol20:class mapCell * advManager::DoAdvCommand(void)@0x0005751b
VA(0x004268d5, 0x619)
class mapCell* advManager::DoAdvCommand(void) {
    signed char moveDone;
    town* viewTown;
    signed char bMoveStopped0;
    hero* selectedHero15;
    int oldMapValid;
    tag_message messageValue8;
    signed char newHover3;
    mapCell* eventCellState16;
    int moveChanged;
    short pathIndex;

    eventCellState16 = NULL;
    selectedHero15 = gpGame->GetHero(gpCurPlayer->m_currentHero);
    bMoveStopped0 = 0;
    newHover3 = 0;
    switch (m_selectedCell) {
    case 1:
        selectedHero15->m_destinationX = m_commandTargetX;
        selectedHero15->m_destinationY = m_commandTargetY;
        goto continue_route;
    case 7:
    continue_route:
        gpSearchArray->BuildPath(
            selectedHero15->m_x,
            selectedHero15->m_y,
            selectedHero15->m_destinationX,
            selectedHero15->m_destinationY,
            999
        );
        if (gpSearchArray->m_pathLength > 0) {
            oldMapValid = m_routeShown;
            MobilizeCurrHero(1);
            if (gConfig.showRoute || oldMapValid)
                ShowRoute(0, 0, 0);
            else if (m_routeShown && m_selectedCell != 7)
                HideRoute(1, 0, 1);
            gpMouseManager->ReallyHidePointer();
            gpInputManager->Flush();
            for (pathIndex = gpSearchArray->m_pathLength - 1; pathIndex >= 0; pathIndex--) {
                eventCellState16 = MoveHero(
                    gpSearchArray->m_directions[pathIndex],
                    pathIndex == 0,
                    &TrigX,
                    &TrigY,
                    &moveChanged,
                    0,
                    &moveDone
                );
                UpdateHeroLocator(-1, 1, 1);
                if (eventCellState16)
                    break;
                if (moveChanged || moveDone)
                    goto movement_done;
                messageValue8 = gpInputManager->GetEvent();
                while (messageValue8.type) {
                    if (messageValue8.type == 1 || messageValue8.type == 8 || messageValue8.type == 0x20
                        || messageValue8.type == 0x200) {
                        bMoveStopped0 = 1;
                        StopCursor(1);
                        goto movement_done;
                    }
                    Process1WindowsMessage();
                    messageValue8 = gpInputManager->GetEvent();
                }
            }
        movement_done:
            if ((pathIndex <= 0 && selectedHero15->m_x == selectedHero15->m_destinationX
                 && selectedHero15->m_y == selectedHero15->m_destinationY)
                || (bMoveStopped0 && !gConfig.showRoute) || eventCellState16)
                HideRoute(1, 1, 1);
            else if (m_selectedCell == 7 || gConfig.showRoute)
                ShowRoute(0, 1, 1);
            gpMouseManager->ReallyShowPointer();
            UpdBottomView(1, 1, 1);
            if (eventCellState16) {
                StopCursor(1);
                DoEvent(eventCellState16, TrigX, TrigY);
                eventCellState16 = NULL;
            }
            Reseed(0, 0);
            newHover3 = 1;
            CheckDimHero();
        }
        break;
    case 6:
        DemobilizeCurrHero();
        gpMouseManager->SetPointer(0);
        viewTown = gpGame->GetTown(selectedHero15->m_occupiedTown);
        viewTown->View();
        eventCellState16 = NULL;
        break;
    case 3:
        DemobilizeCurrHero();
        gpMouseManager->SetPointer(0);
        eventCellState16 = GetCell(
            gpGame->GetTown(gpCurPlayer->m_currentTown)->m_x, gpGame->GetTown(gpCurPlayer->m_currentTown)->m_y
        );
        gpGame->GetTown(gpCurPlayer->m_currentTown)->View();
        eventCellState16 = NULL;
        break;
    case 2:
        gpMouseManager->SetPointer(0);
        gpGame->GetHero(gpCurPlayer->m_currentHero)->HeroView(0);
        RedrawAdvScreen(1);
        gpWindowManager->FadeScreen(0, 8, NULL);
        break;
    case 4:
        SetHeroContext(GetCell(m_mapOriginX + m_lastHoverCell, m_mapOriginY + m_hoverCellY)->m_objectMetadata, 0);
        break;
    case 5:
        SetTownContext(GetCell(m_mapOriginX + m_lastHoverCell, m_mapOriginY + m_hoverCellY)->m_objectMetadata);
        break;
    case -1:
        break;
    }
    m_selectedCell = -1;
    m_lastHoverCell = m_hoverCellY = -1;
    if (newHover3)
        ForceNewHover();
    return eventCellState16;
}

// donor PoL RVA 0x00057d6c; preferred Buka symbol ?Main@advManager@@UAEHAAUtag_message@@@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.507706;margin=0.523825;shape=0.297;size=0.996;calls=0.852;alternate=pol20:int advManager::Main(struct tag_message &);   // virtual [override (implements baseManager pure virtual)]@0x00057d6c
VA(0x00426eee, 0xe10)
short advManager::Main(struct tag_message& message) {
    int yPos;
    int xPos;
    int retVal;
    mapCell* evtMapCell;
    hero* curHero;
    int townIndex;
    int cmdValue;
    int bQuit;
    int moved;
    signed char bEnded;
    int helpText;
    int dir;

    if (KBTickCount() > glTimers[0] && ComboDraw(1))
        UpdateScreen(1, 0);
    if (gbGameOver) {
        message.type = MESSAGE_EXECUTIVE;
        message.executiveCommand = EXECUTIVE_COMMAND_TERMINATE_LOOP;
        return MESSAGE_DISPATCH_FORWARD;
    }
    if (!gbHumanPlayer[giCurPlayer] && (!gbRemoteOn || giHostGamePos == giThisGamePos)) {
        gpPhilAI->DoAI(giCurPlayer);
        gpGame->NextPlayer();
        return MESSAGE_DISPATCH_CONSUME;
    }
    CheckHandleNet();
    if (!gbThisNetHumanPlayer[giCurPlayer])
        return CheckHandleNetPlayerWait(message, 0);
    if (giScreenScroll && gbForegroundApp)
        CheckScreenScroll();
    if (!(message.type & m_messageTypeMask)) {
        if (message.type) {
            message.type = MESSAGE_NONE;
            return MESSAGE_DISPATCH_FORWARD;
        }
        return MESSAGE_DISPATCH_CONTINUE;
    }
    if (!gbNoSound && gConfig.musicVolume && giForceSwitchMusic > 0 && KBTickCount() - giForceSwitchMusic > 6000
        && gpSoundManager->m_currentTrack == 15) {
        giForceSwitchMusic = -1;
        gpSoundManager->SwitchAmbientMusic(m_currentTerrain);
    }
    retVal = MESSAGE_DISPATCH_CONSUME;
    bQuit = 0;
    evtMapCell = NULL;
    if (message.type) {
        switch (message.type) {
        case MESSAGE_WIDGET:
            switch (message.command) {
            case WIDGET_COMMAND_HOVER:
                retVal = ProcessHover(&message);
                break;
            case WIDGET_NOTIFY_DESELECT:
                if (!(message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON))
                    retVal = ProcessDeSelect(&message, &bQuit, &evtMapCell);
                break;
            case WIDGET_NOTIFY_SELECT:
                retVal = ProcessSelect(&message, &evtMapCell);
                break;
            case WIDGET_NOTIFY_RIGHT_CLICK:
                helpText = -1;
                switch (message.id) {
                case 1:
                    helpText = 0;
                    break;
                case 2:
                    helpText = 1;
                    break;
                case 3:
                    helpText = 2;
                    break;
                case 4:
                    helpText = 3;
                    break;
                case 5:
                    helpText = 4;
                    break;
                case 6:
                    helpText = 5;
                    break;
                }
                if (helpText >= 0)
                    NormalDialog(cAdvMenuHelp[helpText], NORMAL_DIALOG_TYPE_QUICK_VIEW, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, -1);
                break;
            }
            break;
        case MESSAGE_KEY_DOWN:
            dir = -1;
            if (gpCurPlayer->CurrentHero() != -1)
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
                    for (cmdValue = 0; cmdValue < HERO_SPELL_SLOT_COUNT; cmdValue++)
                        curHero->AddSpell(cmdValue, 5, 0);
                }
                break;
            case INPUT_SCAN_F7:
                if (curHero)
                    GiveExperience(curHero, 800, 1);
                break;
            case INPUT_SCAN_F8:
                if (curHero) {
                    gpGame->GiveArmy(&curHero->m_army, 23, 1, -1);
                    gpGame->GiveArmy(&curHero->m_army, 10, 1, -1);
                }
                break;
            case INPUT_SCAN_F9:
                for (cmdValue = 0; cmdValue < PLAYER_RESOURCE_COUNT; cmdValue++) {
                    if (cmdValue == 6)
                        gpCurPlayer->m_resources[cmdValue] += 1000;
                    else
                        gpCurPlayer->m_resources[cmdValue] += 10;
                }
                break;
            case INPUT_SCAN_F11:
                if (curHero)
                    curHero->m_remainingMobility = 2999;
                break;
            case INPUT_SCAN_F12:
                gpGame->SetVisibility(30, 30, giCurPlayer, 100);
                UpdateRadar(1, 0);
                CompleteDraw(0);
                UpdateScreen(0, 0);
                break;
            case INPUT_SCAN_0:
                cmdValue = 0;
                goto processCheatDigit;
            case INPUT_SCAN_1:
                cmdValue = 1;
                goto processCheatDigit;
            case INPUT_SCAN_2:
                cmdValue = 2;
                goto processCheatDigit;
            case INPUT_SCAN_3:
                cmdValue = 3;
                goto processCheatDigit;
            case INPUT_SCAN_4:
                cmdValue = 4;
                goto processCheatDigit;
            case INPUT_SCAN_5:
                cmdValue = 5;
                goto processCheatDigit;
            case INPUT_SCAN_6:
                cmdValue = 6;
                goto processCheatDigit;
            case INPUT_SCAN_7:
                cmdValue = 7;
                goto processCheatDigit;
            case INPUT_SCAN_8:
                cmdValue = 8;
                goto processCheatDigit;
            case INPUT_SCAN_9:
                cmdValue = 9;
                goto processCheatDigit;
            processCheatDigit:
                giCheatSeq = giCheatSeq * 10 % 1000000 + cmdValue;
                if (giCheatSeq == 101495) {
                    gpGame->SetVisibility(30, 30, 0, 100);
                    gpGame->SetVisibility(30, 30, 1, 100);
                    gpGame->SetVisibility(30, 30, 2, 100);
                    gpGame->SetVisibility(30, 30, 3, 100);
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
                    ScreenScroll(0, 0);
                else
                    dir = 0;
                break;
            case INPUT_SCAN_NUMPAD_9:
                if (message.modifiers & MESSAGE_MODIFIER_CONTROL_KEYS)
                    ScreenScroll(1, 0);
                else
                    dir = 1;
                break;
            case INPUT_SCAN_NUMPAD_6:
                if (message.modifiers & MESSAGE_MODIFIER_CONTROL_KEYS)
                    ScreenScroll(2, 0);
                else
                    dir = 2;
                break;
            case INPUT_SCAN_NUMPAD_3:
                if (message.modifiers & MESSAGE_MODIFIER_CONTROL_KEYS)
                    ScreenScroll(3, 0);
                else
                    dir = 3;
                break;
            case INPUT_SCAN_NUMPAD_2:
                if (message.modifiers & MESSAGE_MODIFIER_CONTROL_KEYS)
                    ScreenScroll(4, 0);
                else
                    dir = 4;
                break;
            case INPUT_SCAN_NUMPAD_1:
                if (message.modifiers & MESSAGE_MODIFIER_CONTROL_KEYS)
                    ScreenScroll(5, 0);
                else
                    dir = 5;
                break;
            case INPUT_SCAN_NUMPAD_4:
                if (message.modifiers & MESSAGE_MODIFIER_CONTROL_KEYS)
                    ScreenScroll(6, 0);
                else
                    dir = 6;
                break;
            case INPUT_SCAN_NUMPAD_7:
                if (message.modifiers & MESSAGE_MODIFIER_CONTROL_KEYS)
                    ScreenScroll(7, 0);
                else
                    dir = 7;
                break;
            case INPUT_SCAN_C:
                CheckCastSpell();
                break;
            case INPUT_SCAN_D:
                ProcessSearch(-1, -1);
                break;
            case INPUT_SCAN_P:
                ViewPuzzle();
                break;
            case INPUT_SCAN_V:
                ViewWorld(24, 0, 0);
                break;
            case INPUT_SCAN_N:
                cmdValue = 1;
                strcpy(gText, "Are you sure you want to restart?  (Your current game will be lost)");
                goto confirmGameCommand;
            case INPUT_SCAN_L:
                cmdValue = 2;
                strcpy(gText, "Are you sure you want to load a new game?  (Your current game will be lost)");
                goto confirmGameCommand;
            case INPUT_SCAN_Q:
                cmdValue = 4;
                strcpy(gText, "Are you sure you want to quit?");
                goto confirmGameCommand;
            confirmGameCommand:
                bQuit = 1;
                NormalDialog(gText, NORMAL_DIALOG_TYPE_YES_NO, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, -1);
                if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CANCEL)
                    bQuit = 0;
                else
                    gGameCommand = cmdValue;
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
                    if (gpCurPlayer->CurrentTown() == -1) {
                        townIndex = gpCurPlayer->m_townIds[0];
                    } else {
                        townIndex = 0;
                        for (cmdValue = 0; cmdValue < gpCurPlayer->m_townCount; cmdValue++) {
                            if (gpCurPlayer->m_townIds[cmdValue] == gpCurPlayer->CurrentTown()) {
                                if (cmdValue == gpCurPlayer->m_townCount - 1)
                                    townIndex = gpCurPlayer->m_townIds[0];
                                else
                                    townIndex = gpCurPlayer->m_townIds[cmdValue + 1];
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
                if (gpCurPlayer->CurrentTown() != -1) {
                    m_selectedCell = 3;
                    DoAdvCommand();
                } else if (gpCurPlayer->CurrentHero() != -1) {
                    m_selectedCell = 2;
                    DoAdvCommand();
                }
                break;
            }
            if (gpCurPlayer->m_currentHero != -1 && dir >= 0) {
                HideRoute(1, 1, 1);
                gpMouseManager->ReallyHidePointer();
                evtMapCell = MoveHero(dir, 1, &TrigX, &TrigY, &moved, 0, &bEnded);
                UpdateHeroLocator(-1, 1, 1);
                if (evtMapCell) {
                    StopCursor(1);
                    DoEvent(evtMapCell, TrigX, TrigY);
                    evtMapCell = NULL;
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
    if (evtMapCell)
        DoEvent(evtMapCell, TrigX, TrigY);
    if (gbGameOver || bQuit == 1 || giMenuCommand != -1) {
        message.type = MESSAGE_EXECUTIVE;
        message.executiveCommand = EXECUTIVE_COMMAND_TERMINATE_LOOP;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return retVal;
}

// Buka 2.1 Reseed and HoMM1's seven call sites identify this tiny reset.
VA(0x00427cfe, 0x22)
void advManager::Reseed(int, int) {
    giSeedingValid = 0;
}

// donor PoL RVA 0x00058d68; preferred Buka symbol ?ProcessSelect@advManager@@QAEHPAUtag_message@@PAPAVmapCell@@@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.526004;margin=0.306176;shape=0.329;size=0.923;calls=0.956;alternate=pol20:int advManager::ProcessSelect(struct tag_message *, class mapCell * *)@0x00058d68
VA(0x00427d20, 0xe39)
int advManager::ProcessSelect(struct tag_message* message, class mapCell** eventCell) {
    short curX;
    short cellType;
    short mapIndex;
    short curY;
    mapCell* hoverCell;
    int iPage;
    int isVisible;
    tag_message mouseMsg;
    tag_message inputMessage;
    hero* hero;
    signed char mobileResult;

    isVisible = 1;
    switch (message->id) {
    case 105:
    case 112:
    case 119:
    case 126:
        iPage = (message->id - 105) / 7;
        if (gpCurPlayer->m_heroCount <= iPage)
            break;
        cellType = gpCurPlayer->m_heroIds[gpCurPlayer->m_heroLocatorPage + iPage];
        if (message->modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON) {
            HeroQuickView(cellType, iPage, -1, -1);
        } else if (gpCurPlayer->CurrentHero() == cellType) {
            m_selectedCell = 2;
            DoAdvCommand();
        } else {
            HideRoute(1, 0, 1);
            SetHeroContext(cellType, 0);
        }
        break;
    case 16:
    case 17:
    case 18:
    case 19:
        cellType = gpCurPlayer->m_townIds[gpCurPlayer->m_townLocatorPage + message->id - 16];
        if (message->modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON) {
            TownQuickView(cellType, message->id - 16, -1, -1);
        } else {
            HideRoute(1, 0, 1);
            if (gpCurPlayer->CurrentTown() == cellType) {
                m_selectedCell = 3;
                *eventCell = DoAdvCommand();
            } else {
                SetTownContext(cellType);
            }
        }
        break;
    case 20:
        if (gpCurPlayer->m_heroLocatorPage > 0) {
            gpCurPlayer->m_heroLocatorPage--;
            UpdateHeroLocators(1, 1);
        }
        break;
    case 21:
        if (gpCurPlayer->m_heroLocatorPage + 4 < gpCurPlayer->m_heroCount) {
            gpCurPlayer->m_heroLocatorPage++;
            UpdateHeroLocators(1, 1);
        }
        break;
    case 26:
        DoHeroKnob();
        break;
    case 22:
        gpMouseManager->MouseCoords(curX, curY);
        curY -= 194;
        if (gpCurPlayer->m_heroCount > 4) {
            iPage = curY / (92 / (gpCurPlayer->m_heroCount - 3));
            if (iPage > gpCurPlayer->m_heroCount - 4)
                iPage = gpCurPlayer->m_heroCount - 4;
        } else {
            iPage = 0;
        }
        gpCurPlayer->m_heroLocatorPage = iPage;
        UpdateHeroLocators(1, 1);
        break;
    case 27:
        DoTownKnob();
        break;
    case 25:
        gpMouseManager->MouseCoords(curX, curY);
        curY -= 194;
        if (gpCurPlayer->m_townCount > 4) {
            iPage = curY / (92 / (gpCurPlayer->m_townCount - 3));
            if (iPage > gpCurPlayer->m_townCount - 4)
                iPage = gpCurPlayer->m_townCount - 4;
        } else {
            iPage = 0;
        }
        gpCurPlayer->m_townLocatorPage = iPage;
        UpdateTownLocators(1, 1);
        break;
    case 23:
        if (gpCurPlayer->m_townLocatorPage > 0) {
            gpCurPlayer->m_townLocatorPage--;
            UpdateTownLocators(1, 1);
        }
        break;
    case 24:
        if (gpCurPlayer->m_townLocatorPage + 4 < gpCurPlayer->m_townCount) {
            gpCurPlayer->m_townLocatorPage++;
            UpdateTownLocators(1, 1);
        }
        break;
    case 10:
        if (!(gpGame->m_mapExtra[m_lastHoverCell + m_mapOriginX][m_hoverCellY + m_mapOriginY] & giCurPlayerBit))
            isVisible = 0;
        hoverCell = GetCell(m_lastHoverCell + m_mapOriginX, m_hoverCellY + m_mapOriginY);
        if (message->modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON) {
            if (!isVisible) {
                QuickInfo(m_lastHoverCell, m_hoverCellY);
            } else {
                if (m_lastHoverCell == 7 && m_hoverCellY == 7 && gpCurPlayer->CurrentHero() != -1
                    && m_heroContextLocked) {
                    cellType = 0x3d;
                    mapIndex = gpCurPlayer->CurrentHero();
                } else {
                    cellType = hoverCell->m_triggerType & 0x7f;
                    mapIndex = hoverCell->m_objectMetadata;
                }
                switch (cellType) {
                case 0x3d:
                    curX = m_lastHoverCell * 32 - 73;
                    if (curX < 16)
                        curX = 16;
                    if (curX + 178 > 464)
                        curX = 286;
                    curY = m_hoverCellY * 32 - 65;
                    if (curY < 16)
                        curY = 16;
                    if (curY + 162 > 464)
                        curY = 302;
                    HeroQuickView(mapIndex, -1, curX, curY);
                    break;
                case 0x28:
                    curX = m_lastHoverCell * 32 - 89;
                    if (curX < 16)
                        curX = 16;
                    if (curX + 210 > 464)
                        curX = 254;
                    curY = m_hoverCellY * 32 - 70;
                    if (curY < 16)
                        curY = 16;
                    if (curY + 172 > 464)
                        curY = 292;
                    TownQuickView(mapIndex, -1, curX, curY);
                    break;
                default:
                    if (gpGame->m_mapExtra[m_lastHoverCell + m_mapOriginX][m_hoverCellY + m_mapOriginY]
                        & giCurPlayerBit)
                        QuickInfo(m_lastHoverCell, m_hoverCellY);
                    break;
                }
            }
        } else if (isVisible) {
            hero = 0;
            mobileResult = 0;
            if (gpCurPlayer->m_currentHero != -1) {
                hero = gpGame->GetHero(gpCurPlayer->m_currentHero);
                mobileResult = gpGame->IsMobile(hero->m_id);
            }
            if (hero) {
                if (m_lastHoverCell == 7 && m_hoverCellY == 7 && gpCurPlayer->CurrentHero() != -1
                    && m_heroContextLocked) {
                    m_selectedCell = 2;
                    DoAdvCommand();
                } else if ((!mobileResult || (message->modifiers & MESSAGE_MODIFIER_CONTROL_KEYS)
                            || (gConfig.showRoute
                                && (hero->m_destinationX != m_commandTargetX
                                    || hero->m_destinationY != m_commandTargetY)))
                           && gpSearchArray->m_cells[m_commandTargetX][m_commandTargetY].visited) {
                    hero->m_destinationX = m_commandTargetX;
                    hero->m_destinationY = m_commandTargetY;
                    ShowRoute(1, 1, 1);
                } else {
                    *eventCell = DoAdvCommand();
                }
            } else {
                cellType = hoverCell->m_triggerType & 0x7f;
                mapIndex = hoverCell->m_objectMetadata;
                if (cellType == 0x3d) {
                    if (gpCurPlayer->CurrentHero() == mapIndex) {
                        m_selectedCell = 2;
                        DoAdvCommand();
                    } else if (gpGame->GetHero(mapIndex)->m_owner == giCurPlayer) {
                        SetHeroContext(mapIndex, 0);
                    }
                }
                if (cellType == 0x28) {
                    if (gpCurPlayer->CurrentTown() == mapIndex) {
                        m_selectedCell = 3;
                        *eventCell = DoAdvCommand();
                    } else if (gpGame->GetTown(mapIndex)->m_owner == giCurPlayer) {
                        SetTownContext(mapIndex);
                    }
                }
            }
        }
        break;
    case 9:
        if (message->modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON) {
            NormalDialog("World Map (Left click to move viewing area).", 4, -1, -1, -1, 0, -1, 0, -1);
            break;
        }
        DemobilizeCurrHero();
        gpMouseManager->MouseCoords(curX, curY);
        curX = (curX - 480) / 2;
        curY = (curY - 16) / 2;
        m_mapOriginX = curX - 7;
        m_mapOriginY = curY - 7;
        if (m_mapOriginX < -7)
            m_mapOriginX = -7;
        if (m_mapOriginY < -7)
            m_mapOriginY = -7;
        if (m_mapOriginX > 64)
            m_mapOriginX = 64;
        if (m_mapOriginY > 64)
            m_mapOriginY = 64;
        UpdateRadar(1, 0);
        CompleteDraw(0);
        UpdateScreen(0, 0);
        inputMessage.type = MESSAGE_NONE;
        while (inputMessage.type != MESSAGE_LEFT_BUTTON_UP) {
            Process1WindowsMessage();
            inputMessage = gpInputManager->GetEvent();
            mouseMsg = inputMessage;
            while (inputMessage.type != MESSAGE_LEFT_BUTTON_UP && inputMessage.type != MESSAGE_NONE) {
                if (inputMessage.type == MESSAGE_MOUSE_MOVE)
                    mouseMsg = inputMessage;
                Process1WindowsMessage();
                inputMessage = gpInputManager->GetEvent();
            }
            if (mouseMsg.type == MESSAGE_MOUSE_MOVE) {
                if (mouseMsg.x < 480)
                    mouseMsg.x = 480;
                if (mouseMsg.x >= 624)
                    mouseMsg.x = 623;
                if (mouseMsg.y < 16)
                    mouseMsg.y = 16;
                if (mouseMsg.y >= 160)
                    mouseMsg.y = 159;
                gpMouseManager->Main(mouseMsg);
                curX = (mouseMsg.x - 480) / 2;
                curY = (mouseMsg.y - 16) / 2;
                m_mapOriginX = curX - 7;
                m_mapOriginY = curY - 7;
                if (m_mapOriginX < -7)
                    m_mapOriginX = -7;
                if (m_mapOriginY < -7)
                    m_mapOriginY = -7;
                if (m_mapOriginX > 64)
                    m_mapOriginX = 64;
                if (m_mapOriginY > 64)
                    m_mapOriginY = 64;
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
    if ((message->modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON) && message->id >= 2000
        && message->id <= 2200)
        NormalDialog("Status Window\n\nThis window provides information on the status of your hero or kingdom, "
                     "and shows the date.  Left click here to cycle through these windows.",
                     4, -1, -1, -1, 0, -1, 0, -1);
    return 1;
}

// donor PoL RVA 0x00059c19; preferred Buka symbol ?ProcessDeSelect@advManager@@QAEHPAUtag_message@@PAHPAPAVmapCell@@@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.346709;margin=0.551065;shape=0.313;size=0.541;calls=0.500;alternate=pol20:int advManager::ProcessDeSelect(struct tag_message *, int *, class mapCell * *)@0x00059c19
VA(0x00428b59, 0x1ea)
int advManager::ProcessDeSelect(struct tag_message* message, int* result, class mapCell** eventCell) {
    switch (message->id) {
        case 2:
            m_selectedCell = 7;
            *eventCell = DoAdvCommand();
            break;
        case 5:
            AdvPanel();
            break;
        case 6:
            *result = ControlPanel();
            break;
        case 4:
            if (gpCurPlayer->HasMobileHero()) {
                NormalDialog(
                    "One or more Heroes may still move, are you sure you want to end your turn?",
                    NORMAL_DIALOG_TYPE_YES_NO, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, -1
                );
                if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CANCEL)
                    break;
            }
            gpGame->NextPlayer();
            break;
        case 1:
            HideRoute(1, 0, 1);
            SetHeroContext(gpCurPlayer->NextHero(1), 0);
            break;
        case 3:
            gpGame->Overview();
            RedrawAdvScreen(1);
            gpWindowManager->FadeScreen(0, 8, NULL);
            break;
    }
    if (message->id >= 2000 && message->id <= 2200) {
        if (giBottomViewOverride == 2)
            giBottomViewOverride = 1;
        else if (giBottomViewOverride != 0)
            giBottomViewOverride = 0;
        else if (iCurBottomView == 2)
            giBottomViewOverride = 1;
        else
            giBottomViewOverride = 2;
        giBottomViewOverrideEndTime = KBTickCount() + 3000;
        UpdBottomView(1, 1, 1);
    }
    return 1;
}

// donor PoL RVA 0x0005a07c; preferred Buka symbol ?ProcessSearch@advManager@@QAEHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.676641;margin=0.529889;shape=0.365;size=0.904;calls=0.935;strings=%s%s|DIGSOUND.82M;alternate=pol20:int advManager::ProcessSearch(int, int)@0x0005a07c
VA(0x00428d43, 0x49b)
int advManager::ProcessSearch(int x, int y) {
    SAMPLE2 sampleData = NULL_SAMPLE2;
    int gaveArtifact;
    hero* myHero;
    mapCell* pCell;
    tag_message message;
    int i;

    myHero = &gpGame->m_heroRecs[gpCurPlayer->CurrentHero()];
    if (myHero->m_mobility != myHero->m_remainingMobility) {
        NormalDialog("Digging for artifacts requires a whole day, try again tomorrow.", NORMAL_DIALOG_TYPE_OK, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, -1);
        return 1;
    }
    MobilizeCurrHero(0);
    CompleteDraw(0);
    UpdateScreen(0, 0);
    if (x == -1) {
        x = m_mapOriginX + 7;
        y = m_mapOriginY + 7;
    }
    pCell = GetCell(x, y);
    if (pCell->m_objectIndex != 0xff || pCell->m_overlayIndex != 0xff) {
        NormalDialog("Try searching on clear ground.", NORMAL_DIALOG_TYPE_OK, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, -1);
        return 1;
    }
    if (pCell->m_tileIndex < 20) {
        NormalDialog("Try looking on land!!!", NORMAL_DIALOG_TYPE_OK, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, -1);
        return 1;
    }
    if (gbHumanPlayer[giCurPlayer])
        sampleData = LoadPlaySample("DIGSOUND.82M");
    if (pCell->m_objectIndex == 0xff) {
        pCell->m_objectTileset = 7;
        pCell->m_objectIndex = 1;
        pCell->m_flags |= 0x80;
    }
    CompleteDraw(0);
    UpdateScreen(0, 0);
    GrabScreen();

    if (gpGame->m_ultimateArtifactX == x && gpGame->m_ultimateArtifactY == y && gpGame->m_ultimateArtifactId != -1) {
        gaveArtifact = GiveArtifact(myHero, gpGame->m_ultimateArtifactId);
        if (gaveArtifact == -1) {
            NormalDialog("You have no room to carry another artifact!", NORMAL_DIALOG_TYPE_OK, 0x61, 0x28, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, -1);
        } else {
            if (gbHumanPlayer[giCurPlayer]) {
                EventSound(0x3f, 0);
                sprintf(gText, "%s%s",
                        "Congratulations! After spending many hours digging here, you have uncovered the ",
                        gArtifactNames[gpGame->m_ultimateArtifactId]);
                if (gpGame->m_campaignType > 0 && gpGame->m_campaignScenario == 2) {
                    sprintf(gText, "After spending many hours digging here, you have uncovered the Eye of Goros!!!!");
                    NormalDialog(gText, NORMAL_DIALOG_TYPE_OK, 0xb1, 0x1c, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, -1);
                } else {
                    NormalDialog(gText, NORMAL_DIALOG_TYPE_OK, 0xb1, 0x1c, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, -1);
                    myHero->ViewArtifact(gpGame->m_ultimateArtifactId, 0);
                }
                gpSoundManager->SwitchAmbientMusic(m_currentTerrain);
            } else if (gpGame->m_campaignType > 0 && gpGame->m_campaignScenario == 2) {
                sprintf(gText,
                        "A great tragedy - the enemy has found the Eye of Goros!!!  The people abandon you, all is lost.");
                NormalDialog(gText, NORMAL_DIALOG_TYPE_OK, 0xb1, 0x1c, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, -1);
            }
            gpGame->m_ultimateArtifactId = -1;
        }
    } else if (gbHumanPlayer[giCurPlayer]) {
        NormalDialog("Nothing here.", NORMAL_DIALOG_TYPE_OK, 0x61, 0x28, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, -1);
    }
    if (gbHumanPlayer[giCurPlayer])
        WaitEndSample(sampleData, -1);
    for (i = 0; i < gpGame->m_playerCount; i++)
        ComputeUALoc(i);
    myHero->m_remainingMobility = 0;
    UpdBottomView(1, 1, 1);
    CheckDimHero();
    Reseed(0, 0);
    CheckEndGame(0);
    return 1;
}

// DrawCell's per-call drawing state, kept in module storage as in Buka (which
// defines it ahead of its functions). Declared ahead of ProcessHover: the C1
// symbol order retail's ProcessHover and GetCloudLookup operand sorts require
// (docs/patterns/vc4-operand-sort-key-is-the-symbol-handle.md).
int s_drawStoneTile;
int s_drawCovered;
int s_drawCloudFrame;
signed char s_drawFlipCloud;
unsigned short s_drawGroundTile;
unsigned char s_drawTileset;

// donor PoL RVA 0x0005a644; preferred Buka symbol ?ProcessHover@advManager@@QAEHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.464646;margin=0.430920;shape=0.299;size=0.767;calls=0.971;alternate=pol20:int advManager::ProcessHover(int, int)@0x0005a644
VA(0x004291de, 0xc02)
int advManager::ProcessHover(struct tag_message* message) {
    short curX;
    short curY;
    short heroPosX;
    short heroPosY;
    town* pTown;
    hero* hero;
    mapCell* cell;
    int nDays;
    signed char trigType;
    int baseFrame;

    switch (message->id) {
    case 10:
        gpMouseManager->MouseCoords(curX, curY);
        if (curX > 480) {
            gpMouseManager->SetPointer(0);
            return 1;
        }
        curX = curX / 32;
        curY = curY / 32;
        if (curX < 0)
            curX = 0;
        if (curY < 0)
            curY = 0;
        if (curX > 14)
            curX = 14;
        if (curY > 14)
            curY = 14;
        if (m_lastHoverCell != curX || m_hoverCellY != curY) {
            m_selectedCell = -1;
            m_lastHoverCell = curX;
            m_hoverCellY = curY;
            m_commandTargetX = m_mapOriginX + curX;
            m_commandTargetY = m_mapOriginY + curY;
            if (m_commandTargetX < 0 || m_commandTargetY < 0 || m_commandTargetX > MAP_CELL_GRID_SIZE - 1
                || m_commandTargetY > MAP_CELL_GRID_SIZE - 1
                || !(gpGame->m_mapExtra[m_commandTargetX][m_commandTargetY] & giCurPlayerBit)) {
                gpMouseManager->SetPointer(0);
                return 1;
            }
            cell = GetCell(m_commandTargetX, m_commandTargetY);
            if (gpCurPlayer->m_currentHero == -1) {
                if ((cell->m_triggerType & 0x7f) == 0x28
                    && gpGame->GetTown(cell->m_objectMetadata)->m_owner == giCurPlayer) {
                    gpMouseManager->SetPointer(3);
                    m_selectedCell = 3;
                    return 1;
                } else if ((cell->m_triggerType & 0x7f) == 0x3d
                           && gpGame->GetHero(cell->m_objectMetadata)->m_owner == giCurPlayer) {
                    gpMouseManager->SetPointer(2);
                    m_selectedCell = 2;
                    return 1;
                } else {
                    gpMouseManager->SetPointer(0);
                    return 1;
                }
            } else {
                hero = gpGame->GetHero(gpCurPlayer->m_currentHero);
                heroPosX = hero->m_x - m_mapOriginX;
                heroPosY = hero->m_y - m_mapOriginY;
                if (curX == heroPosX && curY == heroPosY) {
                    gpMouseManager->SetPointer(2);
                    m_selectedCell = 2;
                    return 1;
                }
                if (cell->m_unknown07 & 0x80) {
                    if ((cell->m_triggerType & 0x7f) == 0x28) {
                        pTown = gpGame->GetTown(cell->m_objectMetadata);
                        if (pTown->m_owner == giCurPlayer && m_commandTargetY >= 1
                            && m_commandTargetY < MAP_CELL_GRID_SIZE - 1
                            && (GetCell(m_commandTargetX, m_commandTargetY - 1)->m_triggerType & 0x7f) == 0x28
                            && (GetCell(m_commandTargetX, m_commandTargetY + 1)->m_triggerType & 0x7f) == 0x28) {
                            gpMouseManager->SetPointer(3);
                            m_selectedCell = 5;
                            return 1;
                        }
                    }
                    gpSearchArray->m_pathLength = 0;
                    gpMouseManager->SetPointer(0);
                    return 1;
                }
                if (!((m_cursorType == 4 || cell->m_tileIndex >= 20 || cell->m_triggerType == 0xbd
                       || cell->m_triggerType == 0xbe || cell->m_triggerType == 0xa3)
                      && (m_cursorType != 4 || cell->m_tileIndex < 20 || cell->m_triggerType == 0x1f))) {
                    gpSearchArray->m_pathLength = 0;
                    gpMouseManager->SetPointer(0);
                    return 1;
                }
                SeedTo(m_commandTargetX, m_commandTargetY);
                if (gpSearchArray->m_cells[m_commandTargetX][m_commandTargetY].visited) {
                    if (gpSearchArray->m_cells[m_commandTargetX][m_commandTargetY].distance
                        <= hero->m_remainingMobility) {
                        nDays = 0;
                    } else {
                        nDays = (gpSearchArray->m_cells[m_commandTargetX][m_commandTargetY].distance
                                 - hero->m_remainingMobility)
                                    / hero->m_mobility
                                + 1;
                        if (nDays > 3)
                            nDays = 3;
                    }
                    baseFrame = nDays * 6;
                    switch (cell->m_triggerType & 0x7f) {
                    case 0x3e:
                        if (m_cursorType != 4) {
                            gpMouseManager->SetPointer(baseFrame + 6);
                            m_selectedCell = 1;
                        } else {
                            gpMouseManager->SetPointer(baseFrame);
                        }
                        break;
                    case 0x1f:
                        if (m_cursorType == 4)
                            gpMouseManager->SetPointer(baseFrame + 7);
                        else if (mapExtra[m_commandTargetX][m_commandTargetY] & 0x80)
                            gpMouseManager->SetPointer(baseFrame + 5);
                        else
                            gpMouseManager->SetPointer(baseFrame + 4);
                        m_selectedCell = 1;
                        break;
                    case 0x1a:
                        gpMouseManager->SetPointer(baseFrame + 5);
                        m_selectedCell = 1;
                        break;
                    case 0x3d:
                        if (gpGame->GetHero(cell->m_objectMetadata)->m_owner != giCurPlayer) {
                            gpMouseManager->SetPointer(baseFrame + 5);
                            m_selectedCell = 1;
                        } else {
                            gpMouseManager->SetPointer(baseFrame + 8);
                            m_selectedCell = 1;
                        }
                        break;
                    case 0x28:
                        pTown = gpGame->GetTown(cell->m_objectMetadata);
                        if ((cell->m_triggerType & 0x80) && pTown->m_owner != giCurPlayer && pTown->HasGarrison()) {
                            gpMouseManager->SetPointer(baseFrame + 5);
                            m_selectedCell = 1;
                            break;
                        }
                        goto defaultHover;
                    default:
                    defaultHover:
                        trigType = cell->m_triggerType & 0x7f;
                        if ((mapExtra[m_commandTargetX][m_commandTargetY] & 0x80) && m_cursorType != 4
                            && trigType != 4 && trigType != 6 && trigType != 8 && trigType != 0xb
                            && trigType != 0x1d && trigType != 0x30) {
                            gpMouseManager->SetPointer(baseFrame + 5);
                        } else if (cell->m_triggerType & 0x80) {
                            if (m_cursorType != 4) {
                                switch (cell->m_triggerType & 0x7f) {
                                case 1:
                                case 2:
                                case 4:
                                case 5:
                                case 6:
                                case 7:
                                case 8:
                                case 9:
                                case 10:
                                case 11:
                                case 12:
                                case 13:
                                case 14:
                                case 15:
                                case 16:
                                case 17:
                                case 18:
                                case 19:
                                case 20:
                                case 21:
                                case 22:
                                case 23:
                                case 24:
                                case 25:
                                case 27:
                                case 28:
                                case 29:
                                case 32:
                                case 33:
                                case 34:
                                case 35:
                                case 36:
                                case 39:
                                case 40:
                                case 41:
                                case 42:
                                case 43:
                                case 44:
                                case 45:
                                case 46:
                                case 47:
                                case 48:
                                    gpMouseManager->SetPointer(baseFrame + 9);
                                    break;
                                default:
                                    if (mapExtra[m_commandTargetX][m_commandTargetY] & 0x80)
                                        gpMouseManager->SetPointer(baseFrame + 5);
                                    else
                                        gpMouseManager->SetPointer(baseFrame + 4);
                                    break;
                                }
                            } else {
                                switch (cell->m_triggerType & 0x7f) {
                                case 3:
                                case 0x2c:
                                    gpMouseManager->SetPointer(nDays + 28);
                                    break;
                                default:
                                    gpMouseManager->SetPointer(baseFrame + 6);
                                    break;
                                }
                            }
                        } else if (m_cursorType == 4) {
                            gpMouseManager->SetPointer(baseFrame + 6);
                        } else {
                            gpMouseManager->SetPointer(baseFrame + 4);
                        }
                        m_selectedCell = 1;
                        break;
                    }
                    return 1;
                } else {
                    gpMouseManager->SetPointer(0);
                    return 1;
                }
            }
        }
        break;
    default:
        if (!(gpMouseManager->m_cursorFrame >= 32 && gpMouseManager->m_cursorFrame < 40 && MouseInScrollZone()))
            gpMouseManager->SetPointer(0);
        return 1;
    }
    return 1;
}

// donor PoL RVA 0x0005b094; preferred Buka symbol ?UpdateScreen@advManager@@QAEXHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.492392;margin=0.157827;shape=0.294;size=0.928;calls=0.818;alternate=pol20:void advManager::UpdateScreen(int, int)@0x0005b094
VA(0x00429de0, 0x265)
void advManager::UpdateScreen(signed char cursorUpdate, signed char forceUpdate) {
    if (!forceUpdate && !bShowIt) {
        if (KBTickCount() > glTimers[0])
            glTimers[0] = KBTickCount() + 120;
        return;
    }
    gpMouseManager->SaveAndDraw(gpWindowManager->m_screen, m_updateMinX, m_updateMinY, cursorUpdate);
    PollSound();
    giScrollX = m_updateMinX;
    giScrollY = m_updateMinY;
    if (giLimitUpdMinX == -1)
        BlitBitmapToScreen(gpWindowManager->m_screen, 16, 16, 448, 448, 16, 16);
    else
        BlitBitmapToScreen(
            gpWindowManager->m_screen,
            giLimitUpdMinX,
            giLimitUpdMinY,
            giLimitUpdMaxX - giLimitUpdMinX,
            giLimitUpdMaxY - giLimitUpdMinY,
            giLimitUpdMinX,
            giLimitUpdMinY
        );
    giScrollY = 0;
    giScrollX = giScrollY;
    PollSound();
    if (KBTickCount() > glTimers[0]) {
        ++m_updateMaxX;
        if (m_updateMaxX >= 6)
            m_updateMaxX = 0;
        glTimers[0] = KBTickCount() + 120;
        if (m_updateMaxX == 1 || m_updateMaxX == 3 || m_updateMaxX == 5) {
            ++m_animationPhases[1];
            m_animationPhases[1] %= 6;
            ++m_animationPhases[3];
            m_animationPhases[3] %= 6;
        } else {
            ++m_animationPhases[0];
            m_animationPhases[0] %= 6;
            ++m_animationPhases[2];
            m_animationPhases[2] %= 6;
        }
    }
    giLimitUpdMinX = -1;
    gpMouseManager->RestoreUnderlying();
    Process1WindowsMessage();
}

// donor PoL RVA 0x0005b2ae; preferred Buka symbol ?CompleteDraw@advManager@@QAEXHHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:8;base=0.459172;margin=1.327145;shape=0.409;size=0.723;calls=0.706;alternate=pol20:void advManager::CompleteDraw(int, int, int, int)@0x0005b2ae
VA(0x0042a045, 0x359)
void advManager::CompleteDraw(short originX, short originY, int forceDraw) {
    int drawX;
    int drawY;

    PollSound();
    if (!forceDraw && !bShowIt)
        return;

    giLimitUpdMinX = -1;
    m_previousOriginX = m_mapOriginX;
    m_previousOriginY = m_mapOriginY;
    if (gbAllBlack)
        m_mapOriginX = m_mapOriginY = 0;
    m_comboHeroDrawn = 0;
    m_forceCompleteDraw = 0;

    for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++)
        DrawCell(originX + drawX, originY, drawX, 0, ADVMGR_DRAW_GROUND | ADVMGR_DRAW_OBJECT, 0, forceDraw);
    for (drawY = 1; drawY < ADVMGR_VIEW_CELL_COUNT; drawY++)
        for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++)
            DrawCell(originX + drawX, originY + drawY, drawX, drawY, ADVMGR_DRAW_GROUND, 0, forceDraw);

    for (drawY = 1; drawY < ADVMGR_VIEW_CELL_COUNT; drawY++) {
        PollSound();
        if (m_cursorDirection > 4) {
            for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++)
                DrawCell(originX + drawX, originY + drawY - 1, drawX, drawY - 1,
                         ADVMGR_DRAW_OVERLAY | ADVMGR_DRAW_HERO, 0, forceDraw);
        } else {
            for (drawX = ADVMGR_VIEW_CELL_COUNT - 1; drawX >= 0; drawX--)
                DrawCell(originX + drawX, originY + drawY - 1, drawX, drawY - 1,
                         ADVMGR_DRAW_OVERLAY | ADVMGR_DRAW_HERO, 0, forceDraw);
        }
        for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++)
            DrawCell(originX + drawX, originY + drawY, drawX, drawY, ADVMGR_DRAW_OBJECT, 0, forceDraw);
    }

    for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++)
        DrawCell(originX + drawX, originY + ADVMGR_VIEW_CELL_COUNT - 1, drawX, ADVMGR_VIEW_CELL_COUNT - 1,
                 ADVMGR_DRAW_OVERLAY | ADVMGR_DRAW_HERO, 0, forceDraw);
    for (drawY = 0; drawY < ADVMGR_VIEW_CELL_COUNT; drawY++)
        for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++)
            DrawCell(originX + drawX, originY + drawY, drawX, drawY, ADVMGR_DRAW_CLOUD, 0, forceDraw);

    PollSound();
    UpdBottomView(0, 1, 1);
    DrawAdventureBorder();
    if (gbAllBlack) {
        m_mapOriginX = m_previousOriginX;
        m_mapOriginY = m_previousOriginY;
    }
}

// Buka 2.1 CompleteDraw(update) forwards the current map origin.
VA(0x0042a39e, 0x3a)
void advManager::CompleteDraw(int update) {
    CompleteDraw(m_mapOriginX, m_mapOriginY, update);
}

// Buka 2.1 GetCloudLookup over HoMM1's x-major visibility bytes: edge
// masks first, then each unseen neighbour, indexed into the cloud table.
VA(0x0042a3d8, 0x40d)
int advManager::GetCloudLookup(int x, int y) {
    int cloudMask = 0;

    if (x < 1)
        cloudMask |= 0xc8;
    else if (x >= 71)
        cloudMask |= 0x32;
    if (y < 1)
        cloudMask |= 0x91;
    else if (y >= 71)
        cloudMask |= 0x64;
    if (cloudMask == 0) {
        if ((gpGame->m_mapExtra[x][y - 1] & giCurWatchPlayerBit) == 0)
            cloudMask |= 0x01;
        if ((gpGame->m_mapExtra[x + 1][y] & giCurWatchPlayerBit) == 0)
            cloudMask |= 0x02;
        if ((gpGame->m_mapExtra[x][y + 1] & giCurWatchPlayerBit) == 0)
            cloudMask |= 0x04;
        if ((gpGame->m_mapExtra[x - 1][y] & giCurWatchPlayerBit) == 0)
            cloudMask |= 0x08;
        if ((gpGame->m_mapExtra[x + 1][y - 1] & giCurWatchPlayerBit) == 0)
            cloudMask |= 0x10;
        if ((gpGame->m_mapExtra[x + 1][y + 1] & giCurWatchPlayerBit) == 0)
            cloudMask |= 0x20;
        if ((gpGame->m_mapExtra[x - 1][y + 1] & giCurWatchPlayerBit) == 0)
            cloudMask |= 0x40;
        if ((gpGame->m_mapExtra[x - 1][y - 1] & giCurWatchPlayerBit) == 0)
            cloudMask |= 0x80;
    } else {
        if ((cloudMask & 0x01) == 0 && (gpGame->m_mapExtra[x][y - 1] & giCurWatchPlayerBit) == 0)
            cloudMask |= 0x01;
        if ((cloudMask & 0x02) == 0 && (gpGame->m_mapExtra[x + 1][y] & giCurWatchPlayerBit) == 0)
            cloudMask |= 0x02;
        if ((cloudMask & 0x04) == 0 && (gpGame->m_mapExtra[x][y + 1] & giCurWatchPlayerBit) == 0)
            cloudMask |= 0x04;
        if ((cloudMask & 0x08) == 0 && (gpGame->m_mapExtra[x - 1][y] & giCurWatchPlayerBit) == 0)
            cloudMask |= 0x08;
        if ((cloudMask & 0x10) == 0 && (gpGame->m_mapExtra[x + 1][y - 1] & giCurWatchPlayerBit) == 0)
            cloudMask |= 0x10;
        if ((cloudMask & 0x20) == 0 && (gpGame->m_mapExtra[x + 1][y + 1] & giCurWatchPlayerBit) == 0)
            cloudMask |= 0x20;
        if ((cloudMask & 0x40) == 0 && (gpGame->m_mapExtra[x - 1][y + 1] & giCurWatchPlayerBit) == 0)
            cloudMask |= 0x40;
        if ((cloudMask & 0x80) == 0 && (gpGame->m_mapExtra[x - 1][y - 1] & giCurWatchPlayerBit) == 0)
            cloudMask |= 0x80;
    }
    return giCloudType[cloudMask];
}

// donor PoL RVA 0x0005bb7c; preferred Buka symbol ?DrawCell@advManager@@QAEXHHHHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.286321;margin=0.495913;shape=0.238;size=0.410;calls=0.525;alternate=pol20:void advManager::DrawCell(int, int, int, int, int, int)@0x0005bb7c
VA(0x0042a7e5, 0xee8)
void advManager::DrawCell(
    short mapX,
    short mapY,
    short screenX,
    short screenY,
    signed char drawMask,
    signed char drawingPuzzle,
    signed char forceDraw
) {
    signed char frame;
    int heroYOffset6;
    int cursorSuppressed;
    short pixelY3;
    short pixelX7;
    mapCell* cell0;
    hero* drawHero;
    signed char iconIndex;
    signed char flagColor;
    signed char drawHeroIcon0;

    if (!forceDraw && !bShowIt)
        return;
    pixelX7 = screenX << 5;
    pixelY3 = screenY << 5;
    cell0 = GetCell(mapX, mapY);
    if (!gbAllBlack && (mapX < 0 || mapY < 0 || mapX >= 72 || mapY >= 72)) {
        s_drawStoneTile = -1;
        if (mapX == -1) {
            if (mapY == -1)
                s_drawStoneTile = 16;
            else if (mapY == 72)
                s_drawStoneTile = 19;
            else if (mapY >= 0 && mapY < 72)
                s_drawStoneTile = (mapY & 3) + 32;
        } else if (mapX == 72) {
            if (mapY == -1)
                s_drawStoneTile = 17;
            else if (mapY == 72)
                s_drawStoneTile = 18;
            else if (mapY >= 0 && mapY < 72)
                s_drawStoneTile = (mapY & 3) + 24;
        } else if (mapY == -1) {
            if (mapX >= 0 && mapX < 72)
                s_drawStoneTile = (mapX & 3) + 20;
        } else if (mapY == 72 && mapX >= 0 && mapX < 72) {
            s_drawStoneTile = (mapX & 3) + 28;
        }
        if (s_drawStoneTile == -1)
            s_drawStoneTile = (mapX + 16) % 4 + ((mapY + 16) % 4) * 4;
        TileToBitmap(m_stoneTiles, s_drawStoneTile, gpWindowManager->m_screen, pixelX7, pixelY3);
        return;
    } else {
        if (!((!gbAllBlack && (gpGame->m_mapExtra[mapX][mapY] & giCurWatchPlayerBit)) || drawingPuzzle)) {
            s_drawCovered = 1;
            if (gbAllBlack)
                s_drawCloudFrame = 0;
            else
                s_drawCloudFrame = GetCloudLookup(mapX, mapY);
            if (s_drawCloudFrame == 0) {
                if (drawMask & 0x20)
                    TileToBitmap(m_cloudTiles, (mapX + mapY) & 3, gpWindowManager->m_screen, pixelX7, pixelY3);
                return;
            }
            if (s_drawCloudFrame >= 100) {
                s_drawFlipCloud = 1;
                s_drawCloudFrame -= 100;
            } else {
                s_drawFlipCloud = 0;
            }
            if ((s_drawCloudFrame == 1 || s_drawCloudFrame == 5) && (mapX & 1))
                s_drawCloudFrame++;
            if (s_drawCloudFrame == 3 && (mapY & 1))
                s_drawCloudFrame++;
        } else {
            s_drawCovered = 0;
        }
    }
    if (drawMask & 0x20) {
        if (s_drawCovered) {
            if (s_drawFlipCloud)
                FlipIconToBitmap(
                    m_cloudOverlayIcon, gpWindowManager->m_screen, pixelX7 + 31, pixelY3, s_drawCloudFrame - 1, 0
                );
            else
                IconToBitmap(
                    m_cloudOverlayIcon, gpWindowManager->m_screen, pixelX7, pixelY3, s_drawCloudFrame - 1, 0
                );
        } else if (m_routeShown && m_visibilityMap[mapY * 72 + mapX]) {
            if (m_visibilityMap[mapY * 72 + mapX] & 0x20)
                FlipIconToBitmap(
                    m_objectIcons[17], gpWindowManager->m_screen, pixelX7 + 31, pixelY3 + 2,
                    (m_visibilityMap[mapY * 72 + mapX] & 0x1f) - 1, 0
                );
            else
                IconToBitmap(
                    m_objectIcons[17], gpWindowManager->m_screen, pixelX7, pixelY3 + 2,
                    (m_visibilityMap[mapY * 72 + mapX] & 0x1f) - 1, 0
                );
        }
        return;
    }
    if (drawMask & 1) {
        s_drawGroundTile = cell0->m_flags;
        s_drawGroundTile <<= 14;
        s_drawGroundTile |= cell0->m_tileIndex;
        TileToBitmap(m_groundTiles, s_drawGroundTile, gpWindowManager->m_screen, pixelX7, pixelY3);
        if (cell0->m_flags & 0x80) {
            s_drawTileset = cell0->m_objectTileset & 0xf;
            if (!drawingPuzzle || s_drawTileset != 7 || cell0->m_objectIndex != 1)
                IconToBitmap(
                    m_objectIcons[s_drawTileset], gpWindowManager->m_screen, pixelX7, pixelY3,
                    cell0->m_objectIndex, 0
                );
        }
    }
    if (drawMask & 2) {
        if (!(cell0->m_flags & 0x80) && cell0->m_objectIndex != 0xff) {
            s_drawTileset = cell0->m_objectTileset & 0xf;
            if (s_drawTileset != 12) {
                IconToBitmap(
                    m_objectIcons[s_drawTileset], gpWindowManager->m_screen, pixelX7, pixelY3,
                    cell0->m_objectIndex, 0
                );
                if (cell0->m_flags & 4)
                    IconToBitmap(
                        m_objectIcons[s_drawTileset], gpWindowManager->m_screen, pixelX7, pixelY3,
                        cell0->m_objectIndex + m_updateMaxX + 1, 0
                    );
            }
        }
        if (cell0->m_flags & 0x10)
            IconToBitmap(
                m_objectIcons[cell0->m_objectTileset >> 4], gpWindowManager->m_screen, pixelX7,
                pixelY3, cell0->m_unknown05, 0
            );
    }
    if (drawMask & 8) {
        drawHeroIcon0 = 0;
        drawHero = 0;
        if (!(cell0->m_flags & 0x80) && cell0->m_objectIndex != 0xff) {
            s_drawTileset = cell0->m_objectTileset & 0xf;
            if (s_drawTileset == 12 && cell0->m_objectIndex <= 27) {
                if (m_lastQuickViewX == mapX && m_lastQuickViewY == mapY) {
                    if (m_mineGuardianFacingLeft)
                        FlipIconToBitmap(
                            m_objectIcons[20], gpWindowManager->m_screen, pixelX7 + 36, pixelY3 - 5,
                            cell0->m_objectIndex * 7 + 6, 0
                        );
                    else
                        IconToBitmap(
                            m_objectIcons[20], gpWindowManager->m_screen, pixelX7, pixelY3 - 5,
                            cell0->m_objectIndex * 7 + 6, 0
                        );
                } else {
                    ClipIconToBitmap(
                        m_objectIcons[20], gpWindowManager->m_screen, pixelX7, pixelY3 - 5,
                        cell0->m_objectIndex * 7 + m_animationPhases[mapX & 3], 0, 0, 0, 480, 480
                    );
                }
            }
        }
        if (cell0->m_triggerType == 0xbe) {
            flagColor = -1;
            iconIndex = 4;
            frame = GetCursorBaseFrame(gpGame->m_boats[cell0->m_objectMetadata].direction);
            drawHeroIcon0 = 1;
            heroYOffset6 = -10;
        } else {
            heroYOffset6 = 0;
            if (cell0->m_triggerType == 0xbd) {
                drawHero = gpGame->GetHero(cell0->m_objectMetadata);
                if (drawHero->m_eventFlags & 0x80)
                    flagColor = -1;
                else
                    flagColor = gpGame->m_players[drawHero->m_owner].m_unknown11;
                if (drawHero->m_eventFlags & 0x80)
                    iconIndex = 4;
                else
                    iconIndex = drawHero->m_unknown1c;
                frame = GetCursorBaseFrame(drawHero->m_direction);
                drawHeroIcon0 = 1;
                if (drawHero->m_eventFlags & 0x80)
                    heroYOffset6 = -10;
            }
        }
        if (drawHeroIcon0) {
            if (frame & 0x80) {
                if (screenX == 0 || screenY <= 1 || screenX == 14 || screenY == 14) {
                    FlipClippedIconToBitmap(
                        m_heroIcons[iconIndex], gpWindowManager->m_screen, pixelX7 + 32,
                        pixelY3 + 32 - 1 + heroYOffset6, frame & 0x7f, 0
                    );
                    if (flagColor != -1)
                        FlipClippedIconToBitmap(
                            m_flagIcons[flagColor], gpWindowManager->m_screen, pixelX7 + 32,
                            pixelY3 + 32 - 1 + heroYOffset6, frame & 0x7f, 0
                        );
                } else {
                    if (m_drawHeroShadows && iconIndex != 4)
                        FlipDimIconToBitmap(
                            m_boatShadowIcon, gpWindowManager->m_screen, pixelX7 + 32, pixelY3 + 31,
                            frame & 0x7f, 0
                        );
                    FlipIconToBitmap(
                        m_heroIcons[iconIndex], gpWindowManager->m_screen, pixelX7 + 32,
                        pixelY3 + 32 - 1 + heroYOffset6, frame & 0x7f, 0
                    );
                    if (flagColor != -1)
                        FlipIconToBitmap(
                            m_flagIcons[flagColor], gpWindowManager->m_screen, pixelX7 + 32,
                            pixelY3 + 32 - 1 + heroYOffset6, frame & 0x7f, 0
                        );
                }
            } else if (screenX == 0 || screenY <= 1 || screenX == 14 || screenY == 14) {
                ClippedIconToBitmap(
                    m_heroIcons[iconIndex], gpWindowManager->m_screen, pixelX7, pixelY3 + 32 - 1 + heroYOffset6,
                    frame, 0
                );
                if (flagColor != -1)
                    ClippedIconToBitmap(
                        m_flagIcons[flagColor], gpWindowManager->m_screen, pixelX7,
                        pixelY3 + 32 - 1 + heroYOffset6, frame & 0x7f, 0
                    );
            } else {
                if (m_drawHeroShadows && iconIndex != 4)
                    DimIconToBitmap(m_boatShadowIcon, gpWindowManager->m_screen, pixelX7, pixelY3 + 31, frame, 0);
                IconToBitmap(
                    m_heroIcons[iconIndex], gpWindowManager->m_screen, pixelX7, pixelY3 + 32 - 1 + heroYOffset6,
                    frame, 0
                );
                if (flagColor != -1)
                    IconToBitmap(
                        m_flagIcons[flagColor], gpWindowManager->m_screen, pixelX7,
                        pixelY3 + 32 - 1 + heroYOffset6, frame & 0x7f, 0
                    );
            }
        }
        if (m_cursorActive && (cell0->m_flags & 0x40) && !m_comboHeroDrawn && m_mapOriginX + 7 == mapX
            && m_mapOriginY + 7 == mapY) {
            DrawCursor();
            m_comboHeroDrawn = 1;
        }
    }
    if (drawMask & 4) {
        if (cell0->m_overlayIndex != 0xff) {
            s_drawTileset = cell0->m_overlayTileset & 0xf;
            IconToBitmap(
                m_objectIcons[s_drawTileset], gpWindowManager->m_screen, pixelX7, pixelY3,
                cell0->m_overlayIndex, 0
            );
            if (cell0->m_flags & 8)
                IconToBitmap(
                    m_objectIcons[s_drawTileset], gpWindowManager->m_screen, pixelX7, pixelY3,
                    cell0->m_overlayIndex + m_updateMaxX + 1, 0
                );
        }
        if (cell0->m_flags & 0x20)
            IconToBitmap(
                m_objectIcons[cell0->m_overlayTileset >> 4], gpWindowManager->m_screen, pixelX7,
                pixelY3, cell0->m_unknown05, 0
            );
    }
}

// donor PoL RVA 0x0005e0da; preferred Buka symbol ?UpdateRadar@advManager@@QAEXHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:6;base=0.227177;margin=0.921091;shape=0.195;size=0.373;calls=0.222;alternate=pol20:void advManager::UpdateRadar(int, int)@0x0005e0da
// Buka 2.1 GetCell; HoMM1 returns the map base for any off-grid position.
VA(0x0042b6cd, 0x7d)
mapCell* advManager::GetCell(short x, short y) {
    if (x < 0 || y < 0 || x >= MAP_CELL_GRID_SIZE || y >= MAP_CELL_GRID_SIZE)
        return m_mapData[0];
    else
        return &m_mapData[x][y];
}

VA(0x0042b74a, 0x57e)
void advManager::UpdateRadar(signed char updateScreen, int partial) {
    short y;
    int firstX;
    int lastY;
    short x;
    short color;
    short owner;
    int lastX;
    int firstY;
    mapCell* cellPtr;

    if (!partial) {
        firstX = 0;
        firstY = 0;
        lastX = 71;
        lastY = 71;
    } else {
        firstX = m_mapOriginX - 1;
        firstY = m_mapOriginY - 1;
        lastX = m_mapOriginX + 15;
        lastY = m_mapOriginY + 15;
        if (firstX < 0)
            firstX = 0;
        if (firstY < 0)
            firstY = 0;
        if (lastX > 71)
            lastX = 71;
        if (lastY > 71)
            lastY = 71;
    }

    if (!gbThisNetHumanPlayer[giCurPlayer])
        return;

    gpAdvManager->m_openState = 0;
    for (x = firstX; x <= lastX; x++) {
        for (y = firstY; y <= lastY; y++) {
            if (!(gpGame->m_mapExtra[x][y] & giCurPlayerBit)) {
                m_puzzleIcon->FillToBuffer(x * 2 + 480, y * 2 + 16, 0, 0, 0, 0);
                continue;
            }
            cellPtr = &m_mapData[x][y];
            if ((cellPtr->m_triggerType & MAP_TRIGGER_TYPE_MASK) == MAP_OBJECT_HERO) {
                owner = gpGame->m_availableHeroes[cellPtr->m_objectMetadata];
                if (giCurPlayer == owner)
                    color = gRadarOwnerColor[owner >= 0 ? gpGame->m_players[owner].m_unknown11 : 4];
                else
                    color = gRadarTerrainColor[giGroundToTerrain[cellPtr->m_tileIndex]];
            } else {
                switch (cellPtr->m_objectTileset & 0xf) {
                case 10:
                    owner = gpGame->m_townOwners[cellPtr->m_objectMetadata];
                    color = gRadarOwnerColor[owner >= 0 ? gpGame->m_players[owner].m_unknown11 : 4];
                    break;
                case 11:
                    switch (cellPtr->m_triggerType) {
                    case MAP_OBJECT_ALCHEMIST_LAB:
                    case MAP_OBJECT_MINE:
                    case MAP_OBJECT_SAWMILL:
                    case MAP_TRIGGER_EVENT | MAP_OBJECT_ALCHEMIST_LAB:
                    case MAP_TRIGGER_EVENT | MAP_OBJECT_MINE:
                    case MAP_TRIGGER_EVENT | MAP_OBJECT_SAWMILL:
                        owner = gpGame->m_mineOwners[cellPtr->m_objectMetadata];
                        color = gRadarOwnerColor[owner >= 0 ? gpGame->m_players[owner].m_unknown11 : 4];
                        break;
                    default:
                        color = gRadarTerrainColor[giGroundToTerrain[cellPtr->m_tileIndex]];
                        break;
                    }
                    break;
                case 8:
                case 9:
                    color = gRadarTerrainColor[giGroundToTerrain[cellPtr->m_tileIndex]] + 3;
                    break;
                default:
                    color = gRadarTerrainColor[giGroundToTerrain[cellPtr->m_tileIndex]];
                    break;
                }
            }
            m_puzzleIcon->FillToBuffer(x * 2 + 480, y * 2 + 16, 0, color, 0, 0);
        }
    }
    m_puzzleIcon->ClipFillToBuffer(m_mapOriginX * 2 + 480, m_mapOriginY * 2 + 16, 1, 0xbe, 0, 0, 480, 16, 144, 144);
    if (updateScreen)
        gpWindowManager->UpdateScreenRegion(firstX * 2 + 480, firstY * 2 + 16, (lastX - firstX + 1) * 2,
                                            (lastY - firstY + 1) * 2);
}

// donor PoL RVA 0x0005f127; preferred Buka symbol ?QuickInfo@advManager@@QAEXHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.437984;margin=0.160503;shape=0.304;size=0.325;calls=0.543;strings=qwikinfo.bin;alternate=pol20:void advManager::QuickInfo(int, int)@0x0005f127
VA(0x0042bcc8, 0x596)
void advManager::QuickInfo(short cellX, short cellY) {
    short posX;
    tag_message message;
    char savedTextLocal[200];
    mapCell* curCell;
    short posY;
    short flag;
    heroWindow* window;

    flag = 1;
    curCell = NULL;
    posX = cellX * 32 - 57;
    if (posX < 16)
        posX = 16;
    if (posX + 146 > 464)
        posX = 318;
    posY = cellY * 32 - 25;
    if (posY < 16)
        posY = 16;
    if (posY + 82 > 464)
        posY = 382;

    window = new heroWindow(posX, posY, "qwikinfo.bin");
    if (!window)
        MemError();

    if (m_mapOriginX + cellX < 0 || m_mapOriginX + cellX >= 72 || m_mapOriginY + cellY < 0
        || m_mapOriginY + cellY >= 72) {
        sprintf(gText, "\n\n%s", "Border");
    } else {
        curCell = GetCell(m_mapOriginX + cellX, m_mapOriginY + cellY);
        if (!(gpGame->m_mapExtra[m_mapOriginX + cellX][m_mapOriginY + cellY] & giCurPlayerBit)) {
            sprintf(gText, "\n\n%s", "Uncharted territory");
        } else {
            switch (curCell->m_triggerType & MAP_TRIGGER_TYPE_MASK) {
            case MAP_OBJECT_ARTIFACT:
                sprintf(gText, "\n\n%s", "Artifact");
                break;
            case MAP_OBJECT_NONE:
            case MAP_OBJECT_COAST:
            case 50:
                sprintf(gText, "\n\n%s", gTerrainNames[giGroundToTerrain[curCell->m_tileIndex]]);
                break;
            case MAP_OBJECT_MINE:
                sprintf(gText, "\n\n%s %s", gResourceNames[gpGame->m_mines[curCell->m_objectMetadata].type],
                        "Mine");
                break;
            case MAP_OBJECT_RESOURCE:
                sprintf(gText, "\n\n%s", gSpellNames[curCell->m_objectIndex + 9]);
                break;
            case 51:
                sprintf(gText, "\n\n%s", gResourceNames[curCell->m_objectIndex + 2]);
                break;
            case MAP_OBJECT_MONSTER:
                sprintf(gText, "\n\n%s %s", GetArmySizeName(curCell->m_objectMetadata & 0x7f, 1),
                        gArmyNamesPlural[curCell->m_objectIndex]);
                break;
            default:
                sprintf(gText, "\n\n%s", gObjectNames[curCell->m_triggerType & MAP_TRIGGER_TYPE_MASK]);
                break;
            }
        }
    }

    strcpy(savedTextLocal, gText);
    if (giDebugLevel > 0 && curCell)
        sprintf(gText, "otile%d oi%d ot%d ei%d fl%d %s X%d Y%d", curCell->m_objectTileset,
                curCell->m_objectIndex, curCell->m_triggerType, curCell->m_objectMetadata,
                curCell->m_flags & 0x80, savedTextLocal, m_mapOriginX + cellX, m_mapOriginY + cellY);
    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = 1;
    message.text = gText;
    window->BroadcastMessage(message);
    GrabScreen();
    gpWindowManager->AddWindow(window, -1, 1);
    gpMouseManager->HideSystemCursor();
    QuickViewWait();
    gpWindowManager->RemoveWindow(window);
    delete window;
    gpMouseManager->ShowSystemCursor();
}

// donor PoL RVA 0x00060465; preferred Buka symbol ?UpdateHeroLocator@advManager@@QAEXHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.539169;margin=0.099782;shape=0.447;size=0.791;calls=0.917;alternate=pol20:void advManager::UpdateHeroLocator(int, int, int)@0x00060465
VA(0x0042c25e, 0x3c8)
void advManager::UpdateHeroLocator(int locatorSlot, signed char drawWindow, signed char updateScreen) {
    tag_message message;
    signed char whichHero;
    int wBase;
    int i;
    int activeHero;
    hero* hPtr;
    int moveFrame;

    if (!gbThisNetHumanPlayer[giCurPlayer])
        return;
    if (locatorSlot == -1) {
        activeHero = gpCurPlayer->CurrentHero();
        if (activeHero == -1)
            return;
        for (i = 0; i < LOCATOR_VISIBLE_COUNT; i++) {
            if (gpCurPlayer->m_heroIds[gpCurPlayer->m_heroLocatorPage + i] == activeHero)
                locatorSlot = i;
        }
        if (locatorSlot == -1)
            return;
    }
    wBase = locatorSlot * 7 + 100;
    message.type = MESSAGE_WIDGET;
    whichHero = gpCurPlayer->m_heroIds[gpCurPlayer->m_heroLocatorPage + locatorSlot];
    message.command = WIDGET_COMMAND_SET_COLOR;
    message.id = wBase + 6;
    message.value =
        (gpCurPlayer->m_currentHero == whichHero && gpCurPlayer->m_currentHero != -1 && !gbAllBlack)
            ? 0xc5
            : 0;
    m_adventureWindow->BroadcastMessage(message);
    if (whichHero == -1 || gbAllBlack) {
        message.id = wBase + 5;
        message.command = WIDGET_COMMAND_SET_FRAME;
        message.value = locatorSlot;
        m_adventureWindow->BroadcastMessage(message);
        message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.value = WIDGET_FLAG_DRAW;
        for (i = 0; i <= 4; i++) {
            message.id = i + wBase;
            m_adventureWindow->BroadcastMessage(message);
        }
    } else {
        hPtr = gpGame->GetHero(whichHero);
        message.id = wBase + 5;
        message.command = WIDGET_COMMAND_SET_FRAME;
        message.value = 8;
        m_adventureWindow->BroadcastMessage(message);
        message.command = WIDGET_COMMAND_SET_FLAGS;
        message.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
        for (i = 0; i <= 6; i++) {
            message.id = i + wBase;
            m_adventureWindow->BroadcastMessage(message);
        }
        moveFrame = hPtr->m_remainingMobility * 22 / 60;
        if (moveFrame < 0)
            moveFrame = 0;
        if (moveFrame > 30)
            moveFrame = 25;
        else if (moveFrame > 26)
            moveFrame = 24;
        else if (moveFrame > 23)
            moveFrame = 23;
        message.id = wBase + 1;
        message.command = WIDGET_COMMAND_SET_FRAME;
        message.value = moveFrame;
        m_adventureWindow->BroadcastMessage(message);
        message.id = wBase + 2;
        message.command = WIDGET_COMMAND_SET_FRAME;
        message.value = whichHero;
        m_adventureWindow->BroadcastMessage(message);
        message.id = wBase + 3;
        message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
        m_adventureWindow->BroadcastMessage(message);
        message.id = wBase + 4;
        message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
        m_adventureWindow->BroadcastMessage(message);
    }
    if (drawWindow) {
        m_adventureWindow->DrawWindow(0, wBase, wBase + 6);
        if (updateScreen)
            gpWindowManager->UpdateScreenRegion(481, locatorSlot * 32 + 177, 54, 30);
    }
}

// donor PoL RVA 0x000607ad; preferred Buka symbol ?UpdateHeroLocators@advManager@@QAEXHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.520706;margin=0.246406;shape=0.456;size=0.777;calls=0.750;alternate=pol20:void advManager::UpdateHeroLocators(int, int)@0x000607ad
VA(0x0042c626, 0x108)
void advManager::UpdateHeroLocators(signed char drawWindow, signed char updateScreen) {
    int locatorSlot;
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
            static_cast<short>(gpCurPlayer->m_heroLocatorPage * scrollStep + 195.0);
    }
    if (drawWindow)
        m_adventureWindow->DrawWindow(updateScreen);
}

// donor PoL RVA 0x000608af; preferred Buka symbol ?UpdateTownLocators@advManager@@QAEXHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.535916;margin=0.510257;shape=0.379;size=0.971;calls=0.800;alternate=pol20:void advManager::UpdateTownLocators(int, int)@0x000608af
VA(0x0042c72e, 0x27f)
void advManager::UpdateTownLocators(signed char drawWindow, signed char updateScreen) {
    tag_message message;
    short i;
    signed char whichTown;
    double scrollStep;

    if (!gbThisNetHumanPlayer[giCurPlayer])
        return;
    message.type = MESSAGE_WIDGET;
    for (i = 0; i < LOCATOR_VISIBLE_COUNT; i++) {
        whichTown = gpCurPlayer->m_townIds[gpCurPlayer->m_townLocatorPage + i];
        message.command = WIDGET_COMMAND_SET_COLOR;
        message.id = i + 32;
        message.value =
            (gpCurPlayer->m_currentTown != -1 && gpCurPlayer->m_currentTown == whichTown && !gbAllBlack)
                ? 0xc5
                : 0;
        m_adventureWindow->BroadcastMessage(message);
        message.id = i + 16;
        if (whichTown == -1 || gbAllBlack) {
            message.command = WIDGET_COMMAND_SET_FRAME;
            message.value = i + 4;
            m_adventureWindow->BroadcastMessage(message);
            message.command = WIDGET_COMMAND_CLEAR_FLAGS;
            message.value = WIDGET_FLAG_ENABLED;
            m_adventureWindow->BroadcastMessage(message);
        } else {
            message.command = WIDGET_COMMAND_SET_FLAGS;
            message.value = WIDGET_FLAG_ENABLED;
            m_adventureWindow->BroadcastMessage(message);
            message.command = WIDGET_COMMAND_SET_FRAME;
            message.value = gpGame->GetTown(whichTown)->m_type + 12;
            if (gpGame->GetTown(whichTown)->m_buildings & (1 << BUILDING_SLOT_CASTLE))
                message.value += 4;
            m_adventureWindow->BroadcastMessage(message);
        }
    }
    if (gpCurPlayer->m_townCount < 5) {
        m_scrollRightButton->m_y = 232;
    } else {
        scrollStep = 74.0 / (gpCurPlayer->m_townCount - 4);
        m_scrollRightButton->m_y = static_cast<short>(gpCurPlayer->m_townLocatorPage * scrollStep + 195.0);
    }
    if (drawWindow)
        m_adventureWindow->DrawWindow(updateScreen);
}

// donor PoL RVA 0x00060b97; preferred Buka symbol ?UpdBottomView@advManager@@QAEXHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.587774;margin=0.642167;shape=0.494;size=0.843;calls=1.000;alternate=pol20:void advManager::UpdBottomView(int, int, int)@0x00060b97
VA(0x0042c9ad, 0x19f)
void advManager::UpdBottomView(signed char forceUpdate, signed char drawWindow, signed char updateScreen)
{
    signed char updated;

    updated = 0;
    gbForceUpdate = forceUpdate;
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

    if (!gbThisNetHumanPlayer[giCurPlayer] || gbAllBlack)
        updated = UpdBottomViewEnemyTurn();
    else if (gpCurPlayer->CurrentHero() == -1)
        updated = UpdBottomViewKingdom();
    else
        updated = UpdBottomViewHero();

update_bottom_view:
    if (updated && drawWindow) {
        m_adventureWindow->DrawWindow(0, BOTTOM_VIEW_DRAW_FIRST_WIDGET, BOTTOM_VIEW_DRAW_LAST_WIDGET);
        if (updateScreen)
            gpWindowManager->UpdateScreenRegion(BOTTOM_VIEW_PANEL_X, BOTTOM_VIEW_PANEL_Y,
                                                BOTTOM_VIEW_PANEL_WIDTH, BOTTOM_VIEW_PANEL_HEIGHT);
    }
    forceUpdate = gbForceUpdate;
}

// donor PoL RVA 0x00060d63; preferred Buka symbol ?ClearBottomView@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.613154;margin=1.071587;shape=0.489;size=0.961;calls=1.000;alternate=pol20:void advManager::ClearBottomView(void)@0x00060d63
VA(0x0042cb4c, 0x132)
void advManager::ClearBottomView(void)
{
    int widgetIndex;

    if (iCurBottomView == BOTTOM_VIEW_NONE)
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
    iCurBottomViewEnemy = -1;
    iCurBottomView = BOTTOM_VIEW_NONE;
    iLastAnimFrame = -1;
}

// donor PoL RVA 0x00060e95; preferred Buka symbol ?UpdBottomViewEnemyTurn@advManager@@QAEHXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:1;base=0.738388;margin=0.356312;shape=0.508;size=0.910;calls=0.857;strings=brcrest.icn|hourglas.icn|stonback.icn;alternate=pol20:int advManager::UpdBottomViewEnemyTurn(void)@0x00060e95
VA(0x0042cc7e, 0x5bf)
signed char advManager::UpdBottomViewEnemyTurn(void) {
    signed char updated;
    tag_message message;

    updated = 0;
    message.type = MESSAGE_WIDGET;
    if (iCurBottomView != BOTTOM_VIEW_ENEMY_TURN) {
        updated = 1;
        gbForceUpdate = 1;
        ClearBottomView();
        iCurBottomView = BOTTOM_VIEW_ENEMY_TURN;

        m_bottomViewPrimaryWidgets[0] = new iconWidget(BOTTOM_VIEW_PANEL_X, BOTTOM_VIEW_PANEL_Y,
                                                       BOTTOM_VIEW_PANEL_WIDTH, BOTTOM_VIEW_PANEL_HEIGHT,
                                                       "stonback.icn", 0, 0, BOTTOM_VIEW_DRAW_FIRST_WIDGET, 16, 1);
        if (!m_bottomViewPrimaryWidgets[0])
            MemError();
        m_adventureWindow->AddWidget(m_bottomViewPrimaryWidgets[0], 1000);

        m_bottomViewPrimaryWidgets[1] = new iconWidget(493, 403, 118, 51, "hourglas.icn", 0, 0,
                                                       BOTTOM_VIEW_DRAW_FIRST_WIDGET + 1, 16, 1);
        if (!m_bottomViewPrimaryWidgets[1])
            MemError();
        m_adventureWindow->AddWidget(m_bottomViewPrimaryWidgets[1], 1010);
    }

    if (gbForceUpdate || KBTickCount() - iLastSandAnimTime > 300) {
        iLastSandAnimTime = KBTickCount();
        iLastAnimFrame = m_updateMaxX;
        if (KBTickCount() - iLastNewSandAnimTime > 300) {
            iLastNewSandAnimTime = KBTickCount();
            iSandAnim++;
            if (iSandAnim >= 20)
                iSandAnim = 16;
            updated = 1;
            if (m_bottomViewPrimaryWidgets[3]) {
                message.command = WIDGET_COMMAND_SET_FRAME;
                message.id = BOTTOM_VIEW_DRAW_FIRST_WIDGET + 3;
                message.value = iSandAnim + 11;
                m_adventureWindow->BroadcastMessage(message);
            } else {
                m_bottomViewPrimaryWidgets[3] = new iconWidget(559, 405, 50, 47, "hourglas.icn", iSandAnim + 11, 0,
                                                               BOTTOM_VIEW_DRAW_FIRST_WIDGET + 3, 16, 1);
                if (!m_bottomViewPrimaryWidgets[3])
                    MemError();
                m_adventureWindow->AddWidget(m_bottomViewPrimaryWidgets[3], 1020);
            }
        }
    }

    if (gbForceUpdate || iCurBottomViewEnemy != giCurPlayer) {
        updated = 1;
        iCurBottomViewEnemy = giCurPlayer;
        if (iCurBottomViewEnemy != giCurPlayer)
            iCurHourGlassPhase = 0;
        if (m_bottomViewPrimaryWidgets[2]) {
            message.command = WIDGET_COMMAND_SET_FRAME;
            message.id = BOTTOM_VIEW_DRAW_FIRST_WIDGET + 2;
            message.value = gpGame->m_players[giCurPlayer].Color();
            m_adventureWindow->BroadcastMessage(message);
        } else {
            m_bottomViewPrimaryWidgets[2] =
                new iconWidget(495, 405, 50, 47, "brcrest.icn", gpGame->m_players[giCurPlayer].Color(), 0,
                               BOTTOM_VIEW_DRAW_FIRST_WIDGET + 2, 16, 1);
            if (!m_bottomViewPrimaryWidgets[2])
                MemError();
            m_adventureWindow->AddWidget(m_bottomViewPrimaryWidgets[2], 1030);
        }
    }

    if (gbForceUpdate || iCurHourGlassPhase < iLastHourGlassPhase || iLastHourGlassPhase < 0
        || (iCurHourGlassPhase > iLastHourGlassPhase && KBTickCount() - giLastHourGlassUpdateTime >= 700)) {
        updated = 1;
        iLastHourGlassPhase = iCurHourGlassPhase;
        giLastHourGlassUpdateTime = KBTickCount();
        if (m_bottomViewPrimaryWidgets[4]) {
            message.command = WIDGET_COMMAND_SET_FRAME;
            message.id = BOTTOM_VIEW_DRAW_FIRST_WIDGET + 4;
            message.value = iCurHourGlassPhase + 1;
            m_adventureWindow->BroadcastMessage(message);
        } else {
            m_bottomViewPrimaryWidgets[4] = new iconWidget(559, 405, 50, 47, "hourglas.icn", iCurHourGlassPhase + 1,
                                                           0, BOTTOM_VIEW_DRAW_FIRST_WIDGET + 4, 16, 1);
            if (!m_bottomViewPrimaryWidgets[4])
                MemError();
            m_adventureWindow->AddWidget(m_bottomViewPrimaryWidgets[4], 1040);
        }
    }
    return updated;
}

// donor PoL RVA 0x000613b0; preferred Buka symbol ?UpdBottomViewNewTurn@advManager@@QAEHXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.861364;margin=0.145966;shape=0.778;size=0.910;calls=0.840;strings=%s: %d|%s: %d  %s: %d|bigfont.fnt;alternate=pol20:int advManager::UpdBottomViewNewTurn(void)@0x000613b0
VA(0x0042d23d, 0x3e0)
signed char advManager::UpdBottomViewNewTurn(void) {
    int frameIndex;
    int month;
    char* weekStr;
    char* dayStr;

    frameIndex = 0;
    if (!gbForceUpdate && iCurBottomView == BOTTOM_VIEW_NEW_TURN)
        return 0;

    ClearBottomView();
    iCurBottomView = BOTTOM_VIEW_NEW_TURN;
    if (gpGame->m_day == 1 && (gpGame->m_month != 1 || gpGame->m_week != 1 || gpGame->m_day != 1))
        frameIndex = gpGame->m_week;

    m_bottomViewPrimaryWidgets[0] = new iconWidget(BOTTOM_VIEW_PANEL_X, BOTTOM_VIEW_PANEL_Y, 159,
                                                   BOTTOM_VIEW_PANEL_HEIGHT, "stonback.icn", 0, 0,
                                                   BOTTOM_VIEW_DRAW_FIRST_WIDGET, 16, 1);
    if (!m_bottomViewPrimaryWidgets[0])
        MemError();
    m_adventureWindow->AddWidget(m_bottomViewPrimaryWidgets[0], -1);

    m_bottomViewPrimaryWidgets[1] = new iconWidget(BOTTOM_VIEW_PANEL_X, BOTTOM_VIEW_PANEL_Y,
                                                   BOTTOM_VIEW_PANEL_WIDTH, BOTTOM_VIEW_PANEL_HEIGHT,
                                                   "sunmoon.icn", frameIndex, 0,
                                                   BOTTOM_VIEW_DRAW_FIRST_WIDGET + 1, 16, 1);
    if (!m_bottomViewPrimaryWidgets[1])
        MemError();
    m_adventureWindow->AddWidget(m_bottomViewPrimaryWidgets[1], -1);

    weekStr = static_cast<char*>(malloc(30));
    sprintf(weekStr, "%s: %d  %s: %d", "Month", gpGame->m_month, "Week", gpGame->m_week);
    m_bottomViewSecondaryWidgets[0] =
        new textWidget(479, 421, 145, 12, weekStr, "smalfont.fnt", 1, 2100, 512);
    if (!m_bottomViewSecondaryWidgets[0])
        MemError();
    m_adventureWindow->AddWidget(m_bottomViewSecondaryWidgets[0], -1);

    dayStr = static_cast<char*>(malloc(30));
    sprintf(dayStr, "%s: %d", "Day", gpGame->m_day);
    m_bottomViewSecondaryWidgets[0] =
        new textWidget(479, 438, 145, 25, dayStr, "bigfont.fnt", 1, 2100, 512);
    if (!m_bottomViewSecondaryWidgets[0])
        MemError();
    m_adventureWindow->AddWidget(m_bottomViewSecondaryWidgets[0], -1);
    return 1;
}

// donor PoL RVA 0x00061716; preferred Buka symbol ?UpdBottomViewResMsg@advManager@@QAEHXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.790087;margin=0.239399;shape=0.649;size=0.884;calls=0.793;strings=resource.icn|smalfont.fnt|stonback.icn;alternate=pol20:int advManager::UpdBottomViewResMsg(void)@0x00061716
VA(0x0042d61d, 0x3fa)
signed char advManager::UpdBottomViewResMsg(void) {
    int iconW;
    int iconH;
    int y;
    int lineCnt;
    char* messageText;
    char* countString;
    font* smFont;

    if (!gbForceUpdate && iCurBottomView == BOTTOM_VIEW_RESOURCE)
        return 0;

    ClearBottomView();
    iCurBottomView = BOTTOM_VIEW_RESOURCE;
    m_bottomViewPrimaryWidgets[0] = new iconWidget(BOTTOM_VIEW_PANEL_X, BOTTOM_VIEW_PANEL_Y, 159,
                                                   BOTTOM_VIEW_PANEL_HEIGHT, "stonback.icn", 0, 0,
                                                   BOTTOM_VIEW_DRAW_FIRST_WIDGET, 16, 1);
    if (!m_bottomViewPrimaryWidgets[0])
        MemError();
    m_adventureWindow->AddWidget(m_bottomViewPrimaryWidgets[0], -1);

    y = 0;
    if (giBottomViewResource < 0) {
        y = 32;
        smFont = gpResourceManager->GetFont("smalfont.fnt");
        lineCnt = smFont->LineLength(gcBottomViewText, BOTTOM_VIEW_PANEL_WIDTH);
        gpResourceManager->Dispose(smFont);
        y -= lineCnt * 6;
    }
    messageText = static_cast<char*>(malloc(strlen(gcBottomViewText) + 1));
    sprintf(messageText, gcBottomViewText);
    m_bottomViewSecondaryWidgets[0] = new textWidget(480, y + 395, BOTTOM_VIEW_PANEL_WIDTH, 36,
                                                     messageText, "smalfont.fnt", 1, 2100, 512);
    if (!m_bottomViewSecondaryWidgets[0])
        MemError();
    m_adventureWindow->AddWidget(m_bottomViewSecondaryWidgets[0], -1);

    if (giBottomViewResource >= 0) {
        if (giBottomViewResource == 6) {
            iconW = 76;
            iconH = 26;
        } else {
            iconW = 38;
            iconH = 32;
        }
        m_bottomViewPrimaryWidgets[1] =
            new iconWidget((BOTTOM_VIEW_PANEL_WIDTH - iconW) / 2 + 480, 463 - iconH - 14, iconW, iconH,
                           "resource.icn", giBottomViewResource, 0,
                           BOTTOM_VIEW_DRAW_FIRST_WIDGET + 1, 16, 1);
        if (!m_bottomViewPrimaryWidgets[1])
            MemError();
        m_adventureWindow->AddWidget(m_bottomViewPrimaryWidgets[1], -1);

        countString = static_cast<char*>(malloc(8));
        sprintf(countString, "%d", giBottomViewResourceQty);
        m_bottomViewSecondaryWidgets[1] =
            new textWidget(511, 450, 80, 12, countString, "smalfont.fnt", 1, 2101, 512);
        if (!m_bottomViewSecondaryWidgets[1])
            MemError();
        m_adventureWindow->AddWidget(m_bottomViewSecondaryWidgets[1], -1);
    }
    return 1;
}

// donor PoL RVA 0x00061a75; preferred Buka symbol ?UpdBottomViewKingdom@advManager@@QAEHXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.769757;margin=0.069402;shape=0.564;size=0.915;calls=0.850;strings=ressmall.icn|smalfont.fnt|stonback.icn;alternate=pol20:int advManager::UpdBottomViewKingdom(void)@0x00061a75
VA(0x0042da17, 0x3ce)
signed char advManager::UpdBottomViewKingdom(void) {
    int numVillages;
    int i;
    int nCastles;
    signed char rowY[9];
    unsigned char colX[9];
    char* texts[9];

    if (!gbForceUpdate && iCurBottomView == BOTTOM_VIEW_KINGDOM)
        return 0;

    ClearBottomView();
    iCurBottomView = BOTTOM_VIEW_KINGDOM;
    rowY[0] = 59;
    rowY[1] = 59;
    rowY[2] = 59;
    rowY[3] = 59;
    rowY[4] = 59;
    rowY[5] = 59;
    rowY[6] = 28;
    rowY[7] = 28;
    rowY[8] = 28;
    colX[0] = 15;
    colX[1] = 38;
    colX[2] = 61;
    colX[3] = 85;
    colX[4] = 109;
    colX[5] = 132;
    colX[6] = 123;
    colX[7] = 27;
    colX[8] = 80;
    numVillages = 0;
    nCastles = 0;

    m_bottomViewPrimaryWidgets[0] = new iconWidget(BOTTOM_VIEW_PANEL_X, BOTTOM_VIEW_PANEL_Y, 159,
                                                   BOTTOM_VIEW_PANEL_HEIGHT, "stonback.icn", 0, 0,
                                                   BOTTOM_VIEW_DRAW_FIRST_WIDGET, 16, 1);
    if (!m_bottomViewPrimaryWidgets[0])
        MemError();
    m_adventureWindow->AddWidget(m_bottomViewPrimaryWidgets[0], -1);

    m_bottomViewPrimaryWidgets[1] = new iconWidget(481, 393, BOTTOM_VIEW_PANEL_WIDTH, BOTTOM_VIEW_PANEL_HEIGHT,
                                                   "ressmall.icn", 0, 0, BOTTOM_VIEW_DRAW_FIRST_WIDGET + 1,
                                                   16, 1);
    if (!m_bottomViewPrimaryWidgets[1])
        MemError();
    m_adventureWindow->AddWidget(m_bottomViewPrimaryWidgets[1], -1);

    for (i = 0; i < gpCurPlayer->m_townCount; i++) {
        if (gpGame->m_castleRecs[gpCurPlayer->m_townIds[i]].m_buildings & (1 << BUILDING_SLOT_CASTLE))
            nCastles++;
        else
            numVillages++;
    }

    for (i = 0; i < 9; i++) {
        texts[i] = static_cast<char*>(malloc(8));
        if (i < 7)
            sprintf(texts[i], "%d", gpCurPlayer->m_resources[i]);
        else if (i == 7)
            sprintf(texts[i], "%d", nCastles);
        else
            sprintf(texts[i], "%d", numVillages);
        m_bottomViewSecondaryWidgets[i] = new textWidget(colX[i] + 464, rowY[i] + 392, 32, 12, texts[i],
                                                         "smalfont.fnt", 1, i + 2100, 512);
        if (!m_bottomViewSecondaryWidgets[i])
            MemError();
        m_adventureWindow->AddWidget(m_bottomViewSecondaryWidgets[i], -1);
    }
    return 1;
}

// donor PoL RVA 0x00061dd8; preferred Buka symbol ?UpdBottomViewHero@advManager@@QAEHXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.635193;margin=0.117414;shape=0.302;size=0.961;calls=0.625;strings=mons32.icn|smalfont.fnt|stonback.icn;alternate=pol20:int advManager::UpdBottomViewHero(void)@0x00061dd8
VA(0x0042dde5, 0x62c)
signed char advManager::UpdBottomViewHero(void) {
    short slotNum;
    signed char creatureType;
    int n;
    int qtyX;
    char* countStr[5];
    short nStacks;
    short iCrest;
    hero* targetHero;
    int y;
    int x;
    char* heroName;

    if (!gbForceUpdate && iCurBottomView == BOTTOM_VIEW_HERO)
        return 0;

    ClearBottomView();
    iCurBottomView = BOTTOM_VIEW_HERO;
    targetHero = gpGame->GetHero(gpCurPlayer->CurrentHero());
    nStacks = 0;

    m_bottomViewPrimaryWidgets[0] = new iconWidget(BOTTOM_VIEW_PANEL_X, BOTTOM_VIEW_PANEL_Y,
                                                   BOTTOM_VIEW_PANEL_WIDTH, BOTTOM_VIEW_PANEL_HEIGHT,
                                                   "stonback.icn", 0, 0, BOTTOM_VIEW_DRAW_FIRST_WIDGET, 16, 1);
    if (!m_bottomViewPrimaryWidgets[0])
        MemError();
    m_adventureWindow->AddWidget(m_bottomViewPrimaryWidgets[0], -1);

    iCrest = gpCurPlayer->Color() * 4 + targetHero->m_unknown1c;
    m_bottomViewPrimaryWidgets[1] = new iconWidget(495, 395, 25, 25, "smcrest.icn", iCrest, 0,
                                                   BOTTOM_VIEW_DRAW_FIRST_WIDGET + 1, 16, 1);
    if (!m_bottomViewPrimaryWidgets[1])
        MemError();
    m_adventureWindow->AddWidget(m_bottomViewPrimaryWidgets[1], -1);

    heroName = static_cast<char*>(malloc(9));
    strcpy(heroName, targetHero->m_shortName);
    heroName[8] = 0;
    m_bottomViewSecondaryWidgets[0] = new textWidget(475, 418, 66, 12, heroName, "smalfont.fnt", 1, 2100, 512);
    if (!m_bottomViewSecondaryWidgets[0])
        MemError();
    m_adventureWindow->AddWidget(m_bottomViewSecondaryWidgets[0], -1);

    for (n = 0; n < 5; n++) {
        if (targetHero->m_army.m_creatureTypes[n] != CREATURE_NONE)
            nStacks++;
    }
    if (nStacks) {
        slotNum = 0;
        for (n = 0; n < 5; n++) {
            creatureType = targetHero->m_army.m_creatureTypes[n];
            if (creatureType != -1) {
                countStr[slotNum] = static_cast<char*>(malloc(6));
                sprintf(countStr[slotNum], "%d", targetHero->m_army.m_creatureCounts[n]);
                if (slotNum > 2)
                    y = 3;
                else
                    y = 38;
                if (slotNum == 0) {
                    if (nStacks > 2)
                        x = 101;
                    else
                        x = 77;
                } else if (slotNum == 1) {
                    if (nStacks == 2)
                        x = 28;
                    else
                        x = 52;
                } else if (slotNum == 2) {
                    x = 3;
                } else if (slotNum == 3) {
                    if (nStacks == 4)
                        x = 77;
                    else
                        x = 101;
                } else {
                    x = 52;
                }
                m_bottomViewPrimaryWidgets[slotNum + 2] = new iconWidget(x + 480, y + 392, 32, 28, "mons32.icn",
                                                                      creatureType, 0, slotNum + 2002, 16, 1);
                if (!m_bottomViewPrimaryWidgets[slotNum + 2])
                    MemError();
                if (gMons32Width[creatureType] < 28 && strlen(countStr[slotNum]) <= 2)
                    qtyX = x + 30;
                else
                    qtyX = gMons32Width[creatureType] + x + 2;
                m_bottomViewSecondaryWidgets[slotNum + 1] =
                    new textWidget(qtyX + 480, y + 414, strlen(countStr[slotNum]) * 5, 12, countStr[slotNum], "smalfont.fnt",
                                   1, slotNum + 2101, 512);
                if (!m_bottomViewSecondaryWidgets[slotNum + 1])
                    MemError();
                m_adventureWindow->AddWidget(m_bottomViewPrimaryWidgets[slotNum + 2], -1);
                m_adventureWindow->AddWidget(m_bottomViewSecondaryWidgets[slotNum + 1], -1);
                slotNum++;
            }
        }
    }
    return 1;
}

// donor PoL RVA 0x0006235b; preferred Buka symbol ?HeroQuickView@advManager@@QAEXHHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:13;base=0.654225;margin=1.910013;shape=0.309;size=0.949;calls=0.880;strings=mons32.icn|qhero0.bin|qhero1.bin;alternate=pol20:void advManager::HeroQuickView(int, int, int, int)@0x0006235b
VA(0x0042e411, 0xd46)
void advManager::HeroQuickView(signed char heroId, signed char locatorSlot, short windowX, short windowY) {
    short savedOriginX;
    short portraitId;
    short width;
    hero* heroPtr;
    short flagId;
    char* labelText[5];
    short armyW;
    textWidget* sizeTexts[5];
    short j;
    short leftEdge;
    tag_message message;
    short numArmies;
    short creatureY;
    iconWidget* monWidgets[5];
    heroWindow* viewWin;
    short savedOriginY;
    short creatureIconHeight;
    short enable;
    short statWidget;

    armyW = 160;
    leftEdge = 9;
    creatureY = 110;
    width = 32;
    creatureIconHeight = 32;
    enable = 1;
    portraitId = 2;
    statWidget = 3;
    flagId = 8;
    message.type = MESSAGE_WIDGET;
    if (heroId == -1)
        return;
    heroPtr = gpGame->GetHero(heroId);
    if (heroPtr->m_owner == giCurPlayer || m_identifyHeroActive == 1) {
        if (windowX == -1) {
            windowX = 302;
            windowY = locatorSlot * 30 + 111;
        }
        viewWin = new heroWindow(windowX, windowY, "qhero0.bin");
        if (!viewWin)
            MemError();
        SetWinText(viewWin, 9);
    } else {
        viewWin = new heroWindow(windowX, windowY, "qhero1.bin");
        if (!viewWin)
            MemError();
    }

    message.command = WIDGET_COMMAND_SET_FRAME;
    message.id = 2;
    message.value = heroPtr->m_id;
    viewWin->BroadcastMessage(message);
    message.command = WIDGET_COMMAND_SET_FRAME;
    message.id = 8;
    message.value = gpGame->m_players[heroPtr->m_owner].Color() * 2;
    viewWin->BroadcastMessage(message);
    message.id++;
    message.value++;
    viewWin->BroadcastMessage(message);
    sprintf(gText, "%s", heroPtr->m_name);
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = 1;
    message.text = gText;
    viewWin->BroadcastMessage(message);

    numArmies = 0;
    for (j = 0; j < 5; j++) {
        if (heroPtr->m_army.m_creatureTypes[j] != -1)
            numArmies++;
    }

    if (heroPtr->m_owner == giCurPlayer || m_identifyHeroActive == 1) {
        for (j = 0; j < 4; j++) {
            sprintf(gText, "%d", heroPtr->m_primaryStats[j]);
            message.id = j + 3;
            message.text = gText;
            viewWin->BroadcastMessage(message);
        }
        if (numArmies) {
            signed char monster;
            short curIndex;
            short startPos;

            startPos = (160 - numArmies * 32) / 2 + 9;
            curIndex = 0;
            for (j = 0; j < numArmies; j++) {
                while (heroPtr->m_army.m_creatureTypes[curIndex] == -1)
                    curIndex++;
                monster = heroPtr->m_army.m_creatureTypes[curIndex];
                if (monster != -1) {
                    monWidgets[j] = new iconWidget(j * 32 + startPos, 110, 32, 32, "mons32.icn", monster, 0,
                                                       -1, 16, 1);
                    if (!monWidgets[j])
                        MemError();
                    labelText[j] = static_cast<char*>(malloc(5));
                    sprintf(labelText[j], "%d", heroPtr->m_army.m_creatureCounts[curIndex]);
                    sizeTexts[j] = new textWidget(j * 32 + startPos, 140, 32, 12, labelText[j],
                                                        "smalfont.fnt", 1, -1, 512);
                    if (!sizeTexts[j])
                        MemError();
                    viewWin->AddWidget(monWidgets[j], -1);
                    viewWin->AddWidget(sizeTexts[j], -1);
                }
                curIndex++;
            }
        }
    } else if (numArmies) {
        short firstRow;
        short slotIndex;
        signed char creatureId;
        short secondRow;
        short offsetX;
        short step;
        short rowY;

        rowY = 65;
        switch (numArmies) {
        case 1:
        case 2:
        case 3:
            rowY += 22;
            firstRow = numArmies;
            secondRow = 0;
            break;
        case 4:
            firstRow = 2;
            secondRow = 2;
            break;
        default:
            firstRow = 2;
            secondRow = 3;
            break;
        }
        slotIndex = 0;
        step = 160 / firstRow;
        offsetX = (step - 32) / 2 + 9;
        for (j = 0; j < firstRow; j++) {
            while (heroPtr->m_army.m_creatureTypes[slotIndex] == -1)
                slotIndex++;
            creatureId = heroPtr->m_army.m_creatureTypes[slotIndex];
            monWidgets[j] =
                new iconWidget(j * step + offsetX, rowY, 32, 32, "mons32.icn", creatureId, 0, -1, 16, 1);
            if (!monWidgets[j])
                MemError();
            labelText[j] = static_cast<char*>(malloc(15));
            strcpy(labelText[j], GetArmySizeName(heroPtr->m_army.m_creatureCounts[slotIndex], 0));
            sizeTexts[j] = new textWidget(j * step + 9, rowY + 30, step, 12, labelText[j],
                                                "smalfont.fnt", 1, -1, 512);
            if (!sizeTexts[j])
                MemError();
            viewWin->AddWidget(monWidgets[j], -1);
            viewWin->AddWidget(sizeTexts[j], -1);
            slotIndex++;
        }
        if (secondRow) {
            step = 160 / secondRow;
            offsetX = (step - 32) / 2 + 9;
            rowY += 44;
            for (j = firstRow; j < firstRow + secondRow; j++) {
                while (heroPtr->m_army.m_creatureTypes[slotIndex] == -1)
                    slotIndex++;
                creatureId = heroPtr->m_army.m_creatureTypes[slotIndex];
                monWidgets[j] = new iconWidget((j - 2) * step + offsetX, rowY, 32, 32, "mons32.icn",
                                                   creatureId, 0, -1, 16, 1);
                if (!monWidgets[j])
                    MemError();
                labelText[j] = static_cast<char*>(malloc(15));
                strcpy(labelText[j], GetArmySizeName(heroPtr->m_army.m_creatureCounts[slotIndex], 0));
                sizeTexts[j] = new textWidget((j - 2) * step + 9, rowY + 30, step, 12,
                                                    labelText[j], "smalfont.fnt", 1, -1, 512);
                if (!sizeTexts[j])
                    MemError();
                viewWin->AddWidget(monWidgets[j], -1);
                viewWin->AddWidget(sizeTexts[j], -1);
                slotIndex++;
            }
        }
    }

    savedOriginX = m_mapOriginX;
    savedOriginY = m_mapOriginY;
    m_mapOriginX = heroPtr->m_x - 7;
    m_mapOriginY = heroPtr->m_y - 7;
    UpdateRadar(1, 0);
    GrabScreen();
    gpWindowManager->AddWindow(viewWin, -1, 1);
    gpMouseManager->HideSystemCursor();
    QuickViewWait();
    gpWindowManager->RemoveWindow(viewWin);
    delete viewWin;
    gpMouseManager->ShowSystemCursor();
    m_mapOriginX = savedOriginX;
    m_mapOriginY = savedOriginY;
    UpdateRadar(1, 0);
    CompleteDraw(0);
    UpdateScreen(0, 0);
    if (message.type == MESSAGE_LEFT_BUTTON_DOWN && heroPtr->m_owner == giCurPlayer)
        SetHeroContext(heroPtr->m_id, 0);
}

// donor PoL RVA 0x0006308d; preferred Buka symbol ?GetArmySizeName@advManager@@QAEPADHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.463915;margin=0.489878;shape=0.171;size=0.973;calls=1.000;alternate=pol20:char * advManager::GetArmySizeName(int, int)@0x0006308d
VA(0x0042f157, 0xe2)
char* advManager::GetArmySizeName(
    short armySize,
    H1_ENUM_PARAM(ArmySizeNameVariant, signed char) grammar
) {
    if (giDebugLevel > 0) {
        sprintf(cArmySizeName, "%d", armySize);
        return cArmySizeName;
    }
    if (armySize < static_cast<int>(ARMY_FEW_LIMIT))
        return gArmySizeNames[static_cast<int>(ARMY_SIZE_FEW)][static_cast<int>(grammar)];
    if (armySize < static_cast<int>(ARMY_SEVERAL_LIMIT))
        return gArmySizeNames[static_cast<int>(ARMY_SIZE_SEVERAL)][static_cast<int>(grammar)];
    if (armySize < static_cast<int>(ARMY_PACK_LIMIT))
        return gArmySizeNames[static_cast<int>(ARMY_SIZE_PACK)][static_cast<int>(grammar)];
    if (armySize < static_cast<int>(ARMY_LOTS_LIMIT))
        return gArmySizeNames[static_cast<int>(ARMY_SIZE_LOTS)][static_cast<int>(grammar)];
    if (armySize < static_cast<int>(ARMY_HORDE_LIMIT))
        return gArmySizeNames[static_cast<int>(ARMY_SIZE_HORDE)][static_cast<int>(grammar)];
    return gArmySizeNames[static_cast<int>(ARMY_SIZE_ZOUNDS)][static_cast<int>(grammar)];
}

// donor PoL RVA 0x000631ad; preferred Buka symbol ?TownQuickView@advManager@@QAEXHHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:6;base=0.675336;margin=0.051432;shape=0.330;size=0.961;calls=0.941;strings=mons32.icn|qtown1.bin|smalfont.fnt;alternate=pol20:void advManager::TownQuickView(int, int, int, int)@0x000631ad
VA(0x0042f239, 0xc51)
void advManager::TownQuickView(signed char townId, signed char, short windowX, short windowY) {
    short portraitId;
    short creatureIconHeight;
    short numArmies;
    tag_message message;
    short i;
    short flag;
    short flagId;
    short savedOriginX;
    short width;
    heroWindow* viewWin;
    town* townPointer;
    short savedOriginY;
    int detailLevel;
    short armyW;
    short leftEdge;

    armyW = 192;
    leftEdge = 9;
    width = 32;
    creatureIconHeight = 32;
    flag = 1;
    portraitId = 2;
    flagId = 8;
    if (townId == -1)
        return;
    townPointer = gpGame->GetTown(townId);
    if (windowX == -1) {
        windowX = 342;
        windowY = 176;
    }
    viewWin = new heroWindow(windowX, windowY, "qtown1.bin");
    if (!viewWin)
        MemError();
    if (townPointer->m_owner == giCurPlayer) {
        detailLevel = 3;
    } else {
        detailLevel = gpGame->GetNumThievesGuilds(giCurPlayer);
        if (detailLevel > 2)
            detailLevel = 2;
    }
    SetWinText(viewWin, 10);

    numArmies = 0;
    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_FRAME;
    message.id = 2;
    message.value = townPointer->m_type + 12;
    if (gpGame->GetTown(townId)->m_buildings & (1 << BUILDING_SLOT_CASTLE))
        message.value += 4;
    viewWin->BroadcastMessage(message);
    if (townPointer->m_owner == -1) {
        message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.id = 8;
        message.value = WIDGET_FLAG_DRAW;
        viewWin->BroadcastMessage(message);
        message.id++;
        viewWin->BroadcastMessage(message);
    } else {
        message.command = WIDGET_COMMAND_SET_FRAME;
        message.id = 8;
        message.value = gpGame->m_players[townPointer->m_owner].Color() * 2;
        viewWin->BroadcastMessage(message);
        message.id++;
        message.value++;
        viewWin->BroadcastMessage(message);
    }
    sprintf(gText, GetTownName(townPointer->m_id));
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = 1;
    message.text = gText;
    viewWin->BroadcastMessage(message);

    numArmies = 0;
    for (i = 0; i < 5; i++) {
        if (townPointer->m_army.m_creatureTypes[i] != CREATURE_NONE)
            numArmies++;
    }

    if (!detailLevel || !numArmies) {
        char* garrisonStr;
        textWidget* garrisonWidget;

        garrisonStr = static_cast<char*>(malloc(20));
        if (!detailLevel)
            sprintf(garrisonStr, "Unknown");
        else
            sprintf(garrisonStr, "None");
        garrisonWidget = new textWidget(0, 100, 210, 12, garrisonStr, "smalfont.fnt", 1, -1, 512);
        if (!garrisonWidget)
            MemError();
        viewWin->AddWidget(garrisonWidget, -1);
    } else {
        short slotIndex;
        short rowY;
        int xAdjust;
        short offsetX;
        signed char monster;
        short step;
        iconWidget* iconWgts[5];
        short row2;
        short row1;
        textWidget* texts[5];
        signed char dummy;
        char* labels[5];
        signed char slot;

        rowY = 75;
        switch (numArmies) {
        case 1:
        case 2:
        case 3:
            rowY += 22;
            row1 = numArmies;
            row2 = 0;
            break;
        case 4:
            row1 = 2;
            row2 = 2;
            break;
        default:
            row1 = 2;
            row2 = 3;
            break;
        }
        dummy = 0;
        slotIndex = 0;
        slot = 0;
        step = 192 / row1;
        offsetX = (step - 32) / 2 + 9;
        xAdjust = 0;
        for (i = 0; i < row1; i++) {
            if (numArmies == 5) {
                if (i == 0)
                    xAdjust = 12;
                else
                    xAdjust = -12;
            }
            while (townPointer->m_army.m_creatureTypes[slot] == CREATURE_NONE)
                slot++;
            monster = townPointer->m_army.m_creatureTypes[slot];
            iconWgts[slotIndex] = new iconWidget(step * slotIndex + offsetX + xAdjust, rowY, 32, 32,
                                                      "mons32.icn", monster, 0, -1, 16, 1);
            if (!iconWgts[slotIndex])
                MemError();
            labels[slotIndex] = static_cast<char*>(malloc(15));
            if (detailLevel == 3)
                sprintf(labels[slotIndex], "%d", townPointer->m_army.m_creatureCounts[slot]);
            else if (detailLevel == 2)
                strcpy(labels[slotIndex], GetArmySizeName(townPointer->m_army.m_creatureCounts[slot], 0));
            else
                strcpy(labels[slotIndex], "?");
            texts[slotIndex] = new textWidget(step * slotIndex + offsetX + xAdjust - 14, rowY + 30, 60,
                                                       12, labels[slotIndex], "smalfont.fnt", 1, -1, 512);
            if (!texts[slotIndex])
                MemError();
            viewWin->AddWidget(iconWgts[slotIndex], -1);
            viewWin->AddWidget(texts[slotIndex], -1);
            slotIndex++;
            slot++;
        }
        if (row2) {
            step = 192 / row2;
            offsetX = (step - 32) / 2 + 9;
            rowY += 44;
            for (i = row1; i < row1 + row2; i++) {
                while (townPointer->m_army.m_creatureTypes[slot] == CREATURE_NONE)
                    slot++;
                monster = townPointer->m_army.m_creatureTypes[slot];
                iconWgts[slotIndex] = new iconWidget((slotIndex - row1) * step + offsetX, rowY, 32, 32,
                                                          "mons32.icn", monster, 0, -1, 16, 1);
                if (!iconWgts[slotIndex])
                    MemError();
                labels[slotIndex] = static_cast<char*>(malloc(15));
                if (detailLevel == 3)
                    sprintf(labels[slotIndex], "%d", townPointer->m_army.m_creatureCounts[slot]);
                else if (detailLevel == 2)
                    strcpy(labels[slotIndex], GetArmySizeName(townPointer->m_army.m_creatureCounts[slot], 0));
                else
                    strcpy(labels[slotIndex], "?");
                texts[slotIndex] = new textWidget((slotIndex - row1) * step + offsetX - 14,
                                                           rowY + 30, 60, 12, labels[slotIndex], "smalfont.fnt",
                                                           1, -1, 512);
                if (!texts[slotIndex])
                    MemError();
                viewWin->AddWidget(iconWgts[slotIndex], -1);
                viewWin->AddWidget(texts[slotIndex], -1);
                slotIndex++;
                slot++;
            }
        }
    }

    GrabScreen();
    gpWindowManager->AddWindow(viewWin, -1, 1);
    savedOriginX = m_mapOriginX;
    savedOriginY = m_mapOriginY;
    m_mapOriginX = townPointer->m_x - 7;
    m_mapOriginY = townPointer->m_y - 7;
    UpdateRadar(1, 0);
    gpMouseManager->HideSystemCursor();
    QuickViewWait();
    gpWindowManager->RemoveWindow(viewWin);
    delete viewWin;
    gpMouseManager->ShowSystemCursor();
    m_mapOriginX = savedOriginX;
    m_mapOriginY = savedOriginY;
    UpdateRadar(1, 0);
    CompleteDraw(0);
    UpdateScreen(0, 0);
    if (message.type == MESSAGE_LEFT_BUTTON_DOWN && townPointer->m_owner == giCurPlayer)
        SetTownContext(townPointer->m_id);
}

// donor PoL RVA 0x00063dd6; preferred Buka symbol ?RedrawAdvScreen@advManager@@QAEXHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.562981;margin=0.429826;shape=0.444;size=0.906;calls=0.909;alternate=pol20:void advManager::RedrawAdvScreen(int, int)@0x00063dd6
VA(0x0042fe8a, 0xe8)
void advManager::RedrawAdvScreen(int update) {
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

// donor PoL RVA 0x00063f3b; preferred Buka symbol ?MobilizeCurrHero@advManager@@QAEXH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.511468;margin=0.529744;shape=0.406;size=0.742;calls=1.000;alternate=pol20:void advManager::MobilizeCurrHero(int)@0x00063f3b
// Buka 2.1 DeactivateCurrTown clears the current player's town slot.
VA(0x0042ff72, 0x1f)
void advManager::DeactivateCurrTown(void) {
    gpCurPlayer->m_currentTown = -1;
}

// Buka 2.1 DeactivateCurrHero demobilizes before clearing the hero slot.
VA(0x0042ff91, 0x27)
void advManager::DeactivateCurrHero(void) {
    DemobilizeCurrHero();
    gpCurPlayer->m_currentHero = -1;
}

VA(0x0042ffb8, 0x59)
void advManager::MobilizeCurrHero(int update) {
    if (gpCurPlayer->m_currentHero == -1)
        return;
    if (m_heroContextLocked)
        return;
    SetHeroContext(gpCurPlayer->m_currentHero, update);
}

// donor PoL RVA 0x00063f95; preferred Buka symbol ?DemobilizeCurrHero@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.450729;margin=0.647673;shape=0.295;size=0.807;calls=0.800;alternate=pol20:void advManager::DemobilizeCurrHero(void)@0x00063f95
VA(0x00430011, 0x199)
void advManager::DemobilizeCurrHero(void)
{
    if (gpCurPlayer->m_currentHero == -1)
        return;
    if (!m_heroContextLocked)
        return;

    m_heroContextLocked = 0;
    hero *currentHero = gpGame->GetHero(gpCurPlayer->m_currentHero);
    LogInt(currentHero->m_name, currentHero->m_x);
    StopCursor(1);
    currentHero->m_x = m_cursorMapX + m_mapOriginX;
    currentHero->m_y = m_cursorMapY + m_mapOriginY;
    mapCell *cell = GetCell(currentHero->m_x, currentHero->m_y);
    currentHero->m_locationType = cell->m_triggerType;
    currentHero->m_occupiedTown = cell->m_objectMetadata;
    currentHero->m_direction = m_cursorDirection;
    if (m_cursorType == 4)
        currentHero->m_eventFlags |= HERO_EVENT_EMBARKED;
    cell->m_triggerType = (MAP_TRIGGER_EVENT | MAP_OBJECT_HERO);
    cell->m_objectMetadata = currentHero->m_id;
    cell->m_flags &= ~0x40;
    m_cursorActive = 0;
    CompleteDraw(m_mapOriginX, m_mapOriginY, 0);
    UpdateScreen(0, 0);
}

// donor PoL RVA 0x00064101; preferred Buka symbol ?SetTownContext@advManager@@QAEXH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.489027;margin=0.082311;shape=0.312;size=0.827;calls=0.923;alternate=pol20:void advManager::SetTownContext(int)@0x00064101
VA(0x004301aa, 0x255)
void advManager::SetTownContext(signed char townId) {
    short k;
    signed char townNo;
    signed char wasVisible;
    town* townPointer;

    DeactivateCurrHero();
    wasVisible = gpMouseManager->IsVis();
    gpMouseManager->ReallyHidePointer();
    gpCurPlayer->m_currentTown = townId;
    townPointer = gpGame->GetTown(gpCurPlayer->m_currentTown);
    m_mapOriginX = townPointer->m_x - 7;
    m_mapOriginY = townPointer->m_y - 7;
    townNo = 0;
    for (k = 0; k < gpCurPlayer->m_townCount; k++) {
        if (gpCurPlayer->m_townIds[k] == townId)
            townNo = k;
    }
    if (gpCurPlayer->m_townLocatorPage > townNo)
        gpCurPlayer->m_townLocatorPage = townNo;
    else if (gpCurPlayer->m_townLocatorPage + 3 < townNo)
        gpCurPlayer->m_townLocatorPage = townNo - 3;
    UpdateHeroLocators(1, 1);
    UpdateTownLocators(1, 1);
    HideRoute(0, 0, 1);
    UpdBottomView(1, 1, 1);
    UpdateRadar(1, 0);
    CompleteDraw(m_mapOriginX, m_mapOriginY, 0);
    UpdateScreen(0, 0);
    SetEnvironmentOrigin(m_mapOriginX + 7, m_mapOriginY + 7, 1);
    townNo = giGroundToTerrain[GetCell(townPointer->m_x, townPointer->m_y)->m_tileIndex];
    if (m_currentTerrain != townNo) {
        m_currentTerrain = townNo;
        gpSoundManager->SwitchAmbientMusic(m_currentTerrain);
    }
    if (wasVisible)
        gpMouseManager->ReallyShowPointer();
    gpInputManager->m_field_0x34a = 1;
    m_lastHoverCell = 0;
}

// donor PoL RVA 0x00064318; preferred Buka symbol ?SetHeroContext@advManager@@QAEXHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.499995;margin=0.151356;shape=0.325;size=0.844;calls=0.947;alternate=pol20:void advManager::SetHeroContext(int, int)@0x00064318
VA(0x004303ff, 0x3e6)
void advManager::SetHeroContext(signed char heroId, signed char update) {
    signed char wasVisible;
    signed char heroSlot;
    short n;
    mapCell* cellPtr;
    hero* currentHero;

    if (heroId == -1)
        return;
    wasVisible = gpMouseManager->IsVis();
    gpMouseManager->ReallyHidePointer();
    DeactivateCurrTown();
    HideRoute(0, 0, 1);
    DeactivateCurrHero();
    m_heroContextLocked = 1;
    gpCurPlayer->m_currentHero = heroId;
    currentHero = gpGame->GetHero(gpCurPlayer->m_currentHero);
    m_mapOriginX = currentHero->m_x - 7;
    m_mapOriginY = currentHero->m_y - 7;
    m_cursorMapX = m_cursorMapY = 7;
    m_previousCursorMapX = m_previousCursorMapY = -1;
    if (currentHero->m_eventFlags & HERO_EVENT_EMBARKED)
        m_cursorType = 4;
    else
        m_cursorType = currentHero->m_unknown1c;
    m_cursorDirection = currentHero->m_direction;
    m_cursorFrame = GetCursorBaseFrame(m_cursorDirection);
    cellPtr = GetCell(currentHero->m_x, currentHero->m_y);
    cellPtr->m_flags |= 0x40;
    gpGame->RestoreCell(
        currentHero->m_x,
        currentHero->m_y,
        currentHero->m_locationType,
        currentHero->m_occupiedTown,
        NULL,
        4
    );
    heroSlot = 0;
    for (n = 0; n < gpCurPlayer->m_heroCount; n++) {
        if (gpCurPlayer->m_heroIds[n] == heroId)
            heroSlot = n;
    }
    if (gpCurPlayer->m_heroLocatorPage > heroSlot)
        gpCurPlayer->m_heroLocatorPage = heroSlot;
    else if (gpCurPlayer->m_heroLocatorPage + 3 < heroSlot)
        gpCurPlayer->m_heroLocatorPage = heroSlot - 3;
    UpdateHeroLocators(1, 1);
    UpdateTownLocators(1, 1);
    if (!update && (m_active == 1 || gbThisNetHumanPlayer[giCurPlayer])) {
        Reseed(0, 0);
        SeedTo(currentHero->m_destinationX, currentHero->m_destinationY);
        ShowRoute(0, 0, !update);
    }
    UpdBottomView(1, 1, 1);
    m_cursorActive = 1;
    UpdateRadar(1, 0);
    CompleteDraw(m_mapOriginX, m_mapOriginY, 0);
    UpdateScreen(0, 0);
    SetEnvironmentOrigin(m_mapOriginX + 7, m_mapOriginY + 7, 1);
    heroSlot = giGroundToTerrain[cellPtr->m_tileIndex];
    if (m_currentTerrain != heroSlot) {
        m_currentTerrain = heroSlot;
        gpSoundManager->SwitchAmbientMusic(m_currentTerrain);
    }
    if (!gbHeroMoving) {
        if (wasVisible)
            gpMouseManager->ReallyShowPointer();
        gpInputManager->m_field_0x34a = 1;
        m_lastHoverCell = 0;
    }
}

// donor PoL RVA 0x000646aa; preferred Buka symbol ?DoHeroKnob@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.410865;margin=0.380092;shape=0.246;size=0.742;calls=0.733;alternate=pol20:void advManager::DoHeroKnob(void)@0x000646aa
VA(0x004307e5, 0x290)
void advManager::DoHeroKnob(void) {
    double scale;
    short pg;
    tag_message message;
    short numHeroes;
    signed char prevPage;
    short x;
    short offset;
    short my;

    gpMouseManager->SetCursorShape(4);
    prevPage = gpCurPlayer->m_heroLocatorPage;
    numHeroes = gpCurPlayer->m_heroCount;
    scale = 73.0 / (numHeroes - 4);
    gpMouseManager->MouseCoords(x, my);
    offset = my - m_scrollLeftButton->m_y;
    gpInputManager->Flush();
    message = gpInputManager->GetEvent();
    while (message.type != MESSAGE_LEFT_BUTTON_UP && message.type != MESSAGE_RIGHT_BUTTON_UP) {
        if (message.type == MESSAGE_MOUSE_MOVE) {
            if (message.y < offset + 195)
                message.y = offset + 195;
            if (message.y > offset + 195 + 73)
                message.y = offset + 195 + 73;
            gpMouseManager->Main(message);
            m_scrollLeftButton->m_y = message.y - offset;
            m_adventureWindow->DrawWindow();
            if (numHeroes > 4) {
                pg = static_cast<short>((m_scrollLeftButton->m_y - 195) / scale);
                if (prevPage != pg) {
                    gpCurPlayer->m_heroLocatorPage = pg;
                    if (numHeroes - 3 < pg)
                        pg = numHeroes - 3;
                    UpdateHeroLocators(0, 1);
                    m_scrollLeftButton->m_y = message.y - offset;
                    m_adventureWindow->DrawWindow();
                    prevPage = pg;
                }
            }
        }
        Process1WindowsMessage();
        message = gpInputManager->GetEvent();
    }
    gpMouseManager->SetCursorShape(6);
    m_scrollLeftButton->m_flags &= ~1;
    UpdateHeroLocators(1, 1);
}

// donor PoL RVA 0x000648d9; preferred Buka symbol ?DoTownKnob@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.410865;margin=0.000000;shape=0.246;size=0.742;calls=0.733;alternate=pol20:void advManager::DoTownKnob(void)@0x000648d9
VA(0x00430a75, 0x290)
void advManager::DoTownKnob(void) {
    double scale;
    short pg;
    tag_message message;
    short numHeroes;
    signed char prevPage;
    short x;
    short offset;
    short my;

    gpMouseManager->SetCursorShape(4);
    prevPage = gpCurPlayer->m_townLocatorPage;
    numHeroes = gpCurPlayer->m_townCount;
    scale = 73.0 / (numHeroes - 4);
    gpMouseManager->MouseCoords(x, my);
    offset = my - m_scrollRightButton->m_y;
    gpInputManager->Flush();
    message = gpInputManager->GetEvent();
    while (message.type != MESSAGE_LEFT_BUTTON_UP && message.type != MESSAGE_RIGHT_BUTTON_UP) {
        if (message.type == MESSAGE_MOUSE_MOVE) {
            if (message.y < offset + 195)
                message.y = offset + 195;
            if (message.y > offset + 195 + 73)
                message.y = offset + 195 + 73;
            gpMouseManager->Main(message);
            m_scrollRightButton->m_y = message.y - offset;
            m_adventureWindow->DrawWindow();
            if (numHeroes > 4) {
                pg = static_cast<short>((m_scrollRightButton->m_y - 195) / scale);
                if (prevPage != pg) {
                    gpCurPlayer->m_townLocatorPage = pg;
                    if (numHeroes - 3 < pg)
                        pg = numHeroes - 3;
                    UpdateTownLocators(0, 1);
                    m_scrollRightButton->m_y = message.y - offset;
                    m_adventureWindow->DrawWindow();
                    prevPage = pg;
                }
            }
        }
        Process1WindowsMessage();
        message = gpInputManager->GetEvent();
    }
    gpMouseManager->SetCursorShape(6);
    m_scrollRightButton->m_flags &= ~1;
    UpdateTownLocators(1, 1);
}

// donor PoL RVA 0x0006a1dd; preferred Buka symbol ?ViewPuzzle@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.634399;margin=0.712778;shape=0.353;size=0.798;calls=0.917;strings=advmice.mse|puzzle.icn|viewpuzl.bin;alternate=pol20:void advManager::ViewPuzzle(void)@0x0006a1dd
VA(0x00430d05, 0x3da)
void advManager::ViewPuzzle(void) {
    int puzzleX;
    int puzzleY;
    signed char visibleCount;
    icon* puzzlePieces;
    heroWindow* pWin;
    short j;

    visibleCount = 0;
    gpSoundManager->SwitchAmbientMusic(13);
    gpMouseManager->SetPointer("advmice.mse", 0);
    puzzlePieces = gpResourceManager->GetIcon("puzzle.icn");
    for (j = 0; j < 48; j++)
        puzzlePieces->DrawToBuffer(0, 0, j, 0, 0);
    gpWindowManager->UpdateScreenRegion(16, 16, 448, 448);
    gpWindowManager->SaveFizzleSource(16, 16, 448, 448);
    pWin = new heroWindow(480, 16, "viewpuzl.bin");
    if (!pWin)
        MemError();
    gpWindowManager->AddWindow(pWin, -1, 1);

    puzzleX = gpGame->m_ultimateArtifactX - 7;
    puzzleY = gpGame->m_ultimateArtifactY - 7;
    int biasX = 0;
    int biasY = 0;
    biasX = (gpGame->m_ultimateArtifactX + gpGame->m_ultimateArtifactY) % 3 - 1;
    biasY = (gpGame->m_ultimateArtifactY * 5 + gpGame->m_ultimateArtifactX * 2) % 3 - 1;
    if ((gpGame->m_ultimateArtifactX + gpGame->m_ultimateArtifactY) % 3 == 1) {
        if (biasX > 0)
            biasX++;
        else if (biasX < 0)
            biasX--;
    } else if ((gpGame->m_ultimateArtifactX + gpGame->m_ultimateArtifactY) % 2 == 1) {
        if (biasY > 0)
            biasY++;
        else if (biasY < 0)
            biasY--;
    }
    puzzleX += biasX;
    puzzleY += biasY;
    PuzzleDraw(puzzleX, puzzleY, gpGame->m_ultimateArtifactX, gpGame->m_ultimateArtifactY);

    for (j = 0; j < 48; j++) {
        if (!BitTest(gpCurPlayer->m_obelisksVisited, j)) {
            puzzlePieces->DrawToBuffer(0, 0, j, 0, 0);
            visibleCount++;
        }
    }
    if (visibleCount != 48) {
        gpMouseManager->ReallyHidePointer();
        gpWindowManager->FizzleForward(16, 16, 448, 448, 220);
        gpMouseManager->ReallyShowPointer();
    } else {
        gpWindowManager->ReleaseFizzleSource();
    }

    gpWindowManager->DoDialog(pWin, EventWindowHandler, 0);
    delete pWin;
    CompleteDraw(m_mapOriginX, m_mapOriginY, 0);
    UpdateScreen(0, 0);
    UpdateRadar(1, 0);
    gpSoundManager->SwitchAmbientMusic(m_currentTerrain);
}

// HoMM1 PuzzleDraw redraws the 15x15 cells itself, overlaying the puzzle's
// visible object/overlay frames and marking the target cell.
VA(0x004310df, 0x236)
void advManager::PuzzleDraw(int left, int top, int markX, int markY) {
    int y;
    mapCell* cell;
    int x;
    unsigned char tileset;
    short screenX;
    short screenY;

    for (y = 0; y < 15; y++) {
        for (x = 0; x < 15; x++) {
            DrawCell(x + left, top + y, x, y, 1, 1, 0);
            screenX = x * 32;
            screenY = y * 32;
            cell = GetCell(left + x, top + y);
            if (!(cell->m_flags & 0x80) && cell->m_objectIndex != 0xff) {
                tileset = cell->m_objectTileset & 0xf;
                switch (tileset) {
                case 4:
                case 8:
                case 9:
                    IconToBitmap(
                        m_objectIcons[tileset],
                        gpWindowManager->m_screen,
                        screenX,
                        screenY,
                        cell->m_objectIndex,
                        0
                    );
                    break;
                default:
                    break;
                }
            }
            if (cell->m_overlayIndex != 0xff) {
                tileset = cell->m_overlayTileset & 0xf;
                switch (tileset) {
                case 4:
                case 8:
                case 9:
                    IconToBitmap(
                        m_objectIcons[tileset],
                        gpWindowManager->m_screen,
                        screenX,
                        screenY,
                        cell->m_overlayIndex,
                        0
                    );
                    break;
                default:
                    break;
                }
            }
            if (left + x == markX && top + y == markY)
                IconToBitmap(m_objectIcons[17], gpWindowManager->m_screen, screenX, screenY + 2, 13, 0);
        }
    }
    DrawAdventureBorder();
}

// donor PoL RVA 0x00064b08; preferred Buka symbol ?CastSpell@advManager@@QAEXH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:6;base=0.339671;margin=1.295843;shape=0.207;size=0.620;calls=0.615;alternate=pol20:void advManager::CastSpell(int)@0x00064b08
VA(0x00431315, 0x1f2)
void advManager::CastSpell(signed char spell)
{
    hero *caster;
    int guardianCount;

    if (gpCurPlayer->CurrentHero() != -1)
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
            NormalDialog("Enemy Heroes are now fully identifiable.", NORMAL_DIALOG_TYPE_OK, 0x61, 0x91, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, -1);
            break;
        case SPELL_SUMMON_BOAT:
            SummonBoat();
            break;
        case SPELL_DIMENSION_DOOR:
        case SPELL_TOWN_GATE:
            if (caster->m_remainingMobility == 0) {
                NormalDialog("Your hero is too tired to cast this spell today.  Try again tomorrow.",
                             NORMAL_DIALOG_TYPE_OK, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, -1);
                return;
            }
            if (caster->m_remainingMobility < SPELL_TRAVEL_MOBILITY_COST)
                caster->m_remainingMobility = 0;
            else
                caster->m_remainingMobility -= SPELL_TRAVEL_MOBILITY_COST;
            UpdateHeroLocator(-1, 1, 1);
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

// donor PoL RVA 0x00064e9f; preferred Buka symbol ?SaveGame@@YIHXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.731974;margin=0.187030;shape=0.477;size=0.957;calls=0.889;strings=.GM%d|.\GAMES\|advmice.mse;alternate=pol20:int SaveGame(void)@0x00064e9f
// HoMM1's adventure ViewWorld lives in ADVMGR (ground6/flag6/spheres icons);
// CastSpell, AdvPanel, Main and the menu handler pass three signed bytes.
VA(0x00431507, 0x1127)
void advManager::ViewWorld(signed char spellType, signed char drawAllObjects, signed char drawAllTerrains) {
    icon* flags;
    hero* curHero;
    signed char ts;
    signed char flip;
    icon* letters;
    short index;
    heroWindow* win;
    unsigned short mask;
    short x;
    short owner;
    mapCell* cell;
    icon* tilesets[16];
    short i;
    short y;
    short screenX;
    icon* spheres;
    short screenY;
    icon* ground;

    gpMouseManager->SetPointer("advmice.mse", 0);
    mask = 0x300;
    if (spellType == SPELL_VIEW_TOWNS || spellType == SPELL_VIEW_ALL)
        mask |= 0x400;
    ground = gpResourceManager->GetIcon("ground6.icn");
    flags = gpResourceManager->GetIcon("flag6.icn");
    spheres = gpResourceManager->GetIcon("spheres.icn");
    letters = gpResourceManager->GetIcon("letters.icn");
    curHero = NULL;
    for (i = 0; i < 16; i++)
        tilesets[i] = NULL;
    tilesets[9] = gpResourceManager->GetIcon("tree6.icn");
    tilesets[8] = gpResourceManager->GetIcon("mtn6.icn");
    tilesets[10] = gpResourceManager->GetIcon("town6.icn");
    if (gpCurPlayer->CurrentHero() != -1)
        curHero = &gpGame->m_heroRecs[gpCurPlayer->CurrentHero()];
    FillBitmapArea(gpWindowManager->m_screen, 16, 16, 448, 448, 0);

    for (y = 0; y < 72; y++) {
        for (x = 71; x >= 0; x--) {
            cell = GetCell(x, y);
            if ((gpGame->m_mapExtra[x][y] & giCurPlayerBit) || drawAllTerrains
                || (spellType == SPELL_VIEW_TOWNS && (cell->m_triggerType & MAP_TRIGGER_TYPE_MASK) == MAP_OBJECT_TOWN)) {
                flip = 0;
                screenX = x * 6 + 24;
                screenY = y * 6 + 24;
                index = cell->m_tileIndex >> 2;
                if (cell->m_flags & 2)
                    flip = 1;
                if (cell->m_flags & 1)
                    index += 31;
                ground->DrawToBuffer((flip == 1 ? 5 : 0) + screenX, screenY, index, flip, 0);
                if (cell->m_objectIndex != 0xff) {
                    ts = cell->m_objectTileset & 0xf;
                    if (mask & (1 << ts))
                        tilesets[ts]->DrawToBuffer(screenX, screenY, cell->m_objectIndex, 0, 0);
                }
            }
        }
        for (x = 71; x >= 0; x--) {
            cell = GetCell(x, y);
            screenX = x * 6 + 24;
            screenY = y * 6 + 24;
            if ((drawAllObjects || (gpGame->m_mapExtra[x][y] & giCurPlayerBit)) && (cell->m_triggerType & MAP_TRIGGER_EVENT)) {
                switch (spellType) {
                case SPELL_VIEW_ALL:
                    if (cell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_ARTIFACT))
                        flags->DrawToBuffer(screenX, screenY, 6, 0, 0);
                    if (cell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN)) {
                        owner = gpGame->m_townOwners[cell->m_objectMetadata];
                        if (owner >= 0) {
                            index = gpGame->m_players[owner].m_unknown11;
                            flags->DrawToBuffer(screenX - 4, screenY, index, 1, 0);
                            flags->DrawToBuffer(screenX + 3, screenY, index, 0, 0);
                        }
                    } else if (cell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_HERO)
                               && gpGame->m_heroRecs[cell->m_objectMetadata].m_locationType == (MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN)) {
                        owner = gpGame->m_townOwners[gpGame->m_heroRecs[cell->m_objectMetadata].m_occupiedTown];
                        if (owner >= 0) {
                            index = gpGame->m_players[owner].m_unknown11;
                            flags->DrawToBuffer(screenX - 4, screenY, index, 1, 0);
                            flags->DrawToBuffer(screenX + 3, screenY, index, 0, 0);
                        }
                    }
                    switch (cell->m_triggerType & MAP_TRIGGER_TYPE_MASK) {
                    case MAP_OBJECT_ALCHEMIST_LAB:
                    case MAP_OBJECT_MINE:
                    case MAP_OBJECT_SAWMILL:
                        owner = gpGame->m_mineOwners[cell->m_objectMetadata];
                        if (owner >= 0)
                            index = gpGame->m_players[owner].m_unknown11;
                        else
                            index = 4;
                        spheres->DrawToBuffer(screenX, screenY, index, 0, 0);
                        letters->DrawToBuffer(screenX, screenY, gpGame->m_mines[cell->m_objectMetadata].type, 0, 0);
                        break;
                    case MAP_OBJECT_HERO:
                        switch (gpGame->m_heroRecs[cell->m_objectMetadata].m_locationType & MAP_TRIGGER_TYPE_MASK) {
                        case MAP_OBJECT_ALCHEMIST_LAB:
                        case MAP_OBJECT_MINE:
                        case MAP_OBJECT_SAWMILL:
                            owner = gpGame->m_mineOwners[gpGame->m_heroRecs[cell->m_objectMetadata].m_occupiedTown];
                            if (owner >= 0)
                                index = gpGame->m_players[owner].m_unknown11;
                            else
                                index = 4;
                            spheres->DrawToBuffer(screenX, screenY, index, 0, 0);
                            letters->DrawToBuffer(screenX, screenY, gpGame->m_mines[cell->m_objectMetadata].type, 0,
                                                     0);
                            break;
                        default:
                            break;
                        }
                    }
                    if (cell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_HERO)) {
                        owner = gpGame->m_availableHeroes[cell->m_objectMetadata];
                        if (owner >= 0) {
                            index = gpGame->m_players[owner].m_unknown11;
                            flags->DrawToBuffer(screenX, screenY, index, 0, 0);
                        }
                    }
                    break;
                case SPELL_VIEW_MINES:
                    switch (cell->m_triggerType & MAP_TRIGGER_TYPE_MASK) {
                    case MAP_OBJECT_ALCHEMIST_LAB:
                    case MAP_OBJECT_MINE:
                    case MAP_OBJECT_SAWMILL:
                        owner = gpGame->m_mineOwners[cell->m_objectMetadata];
                        if (owner >= 0)
                            index = gpGame->m_players[owner].m_unknown11;
                        else
                            index = 4;
                        spheres->DrawToBuffer(screenX, screenY, index, 0, 0);
                        letters->DrawToBuffer(screenX, screenY, gpGame->m_mines[cell->m_objectMetadata].type, 0, 0);
                        break;
                    case MAP_OBJECT_HERO:
                        switch (gpGame->m_heroRecs[cell->m_objectMetadata].m_locationType & MAP_TRIGGER_TYPE_MASK) {
                        case MAP_OBJECT_ALCHEMIST_LAB:
                        case MAP_OBJECT_MINE:
                        case MAP_OBJECT_SAWMILL:
                            owner = gpGame->m_mineOwners[gpGame->m_heroRecs[cell->m_objectMetadata].m_occupiedTown];
                            if (owner >= 0)
                                index = gpGame->m_players[owner].m_unknown11;
                            else
                                index = 4;
                            spheres->DrawToBuffer(screenX, screenY, index, 0, 0);
                            letters->DrawToBuffer(screenX, screenY, gpGame->m_mines[cell->m_objectMetadata].type, 0,
                                                     0);
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
                    if (cell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_RESOURCE)) {
                        spheres->DrawToBuffer(screenX - 3, screenY, 4, 0, 0);
                        letters->DrawToBuffer(screenX - 3, screenY, cell->m_objectIndex - 0x3d, 0, 0);
                    }
                    break;
                case SPELL_VIEW_ARTIFACTS:
                    if (cell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_ARTIFACT))
                        flags->DrawToBuffer(screenX, screenY, 6, 0, 0);
                    break;
                case SPELL_VIEW_TOWNS:
                    if (cell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN)) {
                        owner = gpGame->m_townOwners[cell->m_objectMetadata];
                        if (owner >= 0) {
                            index = gpGame->m_players[owner].m_unknown11;
                            flags->DrawToBuffer(screenX - 4, screenY, index, 1, 0);
                            flags->DrawToBuffer(screenX + 3, screenY, index, 0, 0);
                        }
                    } else if (cell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_HERO)
                               && gpGame->m_heroRecs[cell->m_objectMetadata].m_locationType == (MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN)) {
                        owner = gpGame->m_townOwners[gpGame->m_heroRecs[cell->m_objectMetadata].m_occupiedTown];
                        if (owner >= 0) {
                            index = gpGame->m_players[owner].m_unknown11;
                            flags->DrawToBuffer(screenX - 4, screenY, index, 1, 0);
                            flags->DrawToBuffer(screenX + 3, screenY, index, 0, 0);
                        }
                    }
                    break;
                case SPELL_VIEW_HEROES:
                    if (cell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_HERO)) {
                        owner = gpGame->m_availableHeroes[cell->m_objectMetadata];
                        if (owner >= 0) {
                            index = gpGame->m_players[owner].m_unknown11;
                            flags->DrawToBuffer(screenX, screenY, index, 0, 0);
                        }
                    }
                    break;
                default:
                    break;
                }
            }
            if (curHero && curHero->m_x == x && curHero->m_y == y)
                flags->DrawToBuffer(screenX, screenY, 5, 0, 0);
        }
        for (x = 71; x >= 0; x--) {
            cell = GetCell(x, y);
            if ((gpGame->m_mapExtra[x][y] & giCurPlayerBit) || drawAllTerrains
                || (cell->m_triggerType == MAP_OBJECT_TOWN && spellType == SPELL_VIEW_TOWNS)) {
                screenX = x * 6 + 24;
                screenY = y * 6 + 24;
                if (cell->m_overlayIndex != 0xff) {
                    ts = cell->m_overlayTileset & 0xf;
                    if (mask & (1 << ts))
                        tilesets[ts]->DrawToBuffer(screenX, screenY, cell->m_overlayIndex, 0, 0);
                }
            }
        }
    }

    gpWindowManager->UpdateScreenRegion(16, 16, 448, 448);
    sprintf(gText, "view-%02d.bin", spellType - SPELL_VIEW_MINES);
    win = new heroWindow(480, 16, gText);
    if (!win)
        MemError();
    gpWindowManager->DoDialog(win, TrueFalseDialogHandler, 0);
    delete win;
    UpdateRadar(1, 0);
    for (i = 0; i < 16; i++) {
        if (tilesets[i])
            gpResourceManager->Dispose(tilesets[i]);
    }
    gpResourceManager->Dispose(ground);
    gpResourceManager->Dispose(flags);
    gpResourceManager->Dispose(spheres);
    gpResourceManager->Dispose(letters);
    RedrawAdvScreen(1);
}

// HoMM1-only helper: refresh the saved screen copy with the pointer hidden.
VA(0x0043262e, 0x41)
void advManager::GrabScreen(void) {
    gpMouseManager->ReallyHidePointer();
    GrabScreenBitmap(gpWindowManager->m_screen, 0, 0);
    gpMouseManager->ReallyShowPointer();
}

// clang-format off
H1_ENUM_CONST_BEGIN(ControlPanelDialogConstant)
    CONTROL_NEW_GAME = 1,
    CONTROL_LOAD_GAME = 2,
    CONTROL_SAVE_GAME = 3,
    CONTROL_QUIT = 4,
    CONTROL_MUSIC_VOLUME = 5,
    CONTROL_SOUND_VOLUME = 6,
    CONTROL_WALK_SPEED = 7,
    CONTROL_MUSIC_SOURCE = 11,
    CONTROL_SHOW_ROUTE = 12,
    CONTROL_SHOW_ENEMY_MOVES = 13,
    CONTROL_SCENARIO_INFO = 17
H1_ENUM_CONST_END(ControlPanelDialogConstant)
// clang-format on

// HoMM1 merges Buka's ControlPanel and SystemOptions: one cpanel.bin dialog
// that also applies the walk-speed sample set and saves changed preferences.
VA(0x0043266f, 0x321)
short advManager::ControlPanel(void) {
    tag_message message;
    int mobilized;
    signed char oldSpeed;
    int gameCommand;
    int n;

    TrimLoopingSounds(ADVMGR_ACTIVE_SOUND_COUNT);
    gpMouseManager->SetPointer("advmice.mse", 0);
    gameCommand = -1;
    oldSpeed = gConfig.walkSpeed;
    bFreshSave = 0;
    mobilized = m_heroContextLocked;
    bPrefsChanged = 0;
    DemobilizeCurrHero();
    cPanel = new heroWindow(160, 10, "cpanel.bin");
    if (cPanel == NULL)
        MemError();
    SetWinText(cPanel, 3);
    if (gbRemoteOn) {
        message.type = MESSAGE_WIDGET;
        message.id = CONTROL_NEW_GAME;
        message.command = WIDGET_COMMAND_SET_FLAGS;
        message.value = WIDGET_COMMAND_DIMMED;
        cPanel->BroadcastMessage(message);
        message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.value = WIDGET_FLAG_ENABLED;
        cPanel->BroadcastMessage(message);
        message.id = CONTROL_LOAD_GAME;
        message.command = WIDGET_COMMAND_SET_FLAGS;
        message.value = WIDGET_COMMAND_DIMMED;
        cPanel->BroadcastMessage(message);
        message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.value = WIDGET_FLAG_ENABLED;
        cPanel->BroadcastMessage(message);
    }
    UpdateCPanel(1);
    gpWindowManager->DoDialog(cPanel, CPanelHandler, 0);
    delete cPanel;
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
    if (oldSpeed != gConfig.walkSpeed) {
        for (n = 0; n < ADVMGR_CURSOR_SAMPLE_COUNT; n++)
            gpResourceManager->Dispose(m_cursorSamples[n]);
        GetCursorSampleSet(gConfig.walkSpeed);
    }
    if (bPrefsChanged)
        WritePrefs();
    if (mobilized)
        MobilizeCurrHero(0);
    if (gameCommand != -1) {
        gGameCommand = gameCommand;
        return 1;
    }
    return 0;
}

extern char *onOffText[];
extern char *walkSpeedText[];
extern char *musicQualityText[];

// Buka 2.1 UpdateSystemOptions over HoMM1's six control-panel options.
VA(0x00432990, 0x227)
void UpdateCPanel(signed char initialDraw) {
    tag_message message;

    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_FRAME;
    message.id = CONTROL_MUSIC_VOLUME;
    message.value = gConfig.musicVolume ? 11 : 10;
    cPanel->BroadcastMessage(message);
    message.id = CONTROL_SOUND_VOLUME;
    message.value = gConfig.soundVolume ? 13 : 12;
    cPanel->BroadcastMessage(message);
    message.id = CONTROL_WALK_SPEED;
    message.value = gConfig.walkSpeed + 14;
    cPanel->BroadcastMessage(message);
    message.id = CONTROL_MUSIC_SOURCE;
    message.value = gConfig.musicSource + 27;
    cPanel->BroadcastMessage(message);
    message.id = CONTROL_SHOW_ROUTE;
    message.value = gConfig.showRoute + 21;
    cPanel->BroadcastMessage(message);
    message.id = CONTROL_SHOW_ENEMY_MOVES;
    message.value = gbRemoteOn ? 23 : 1 - gConfig.blackoutComputer + 23;
    cPanel->BroadcastMessage(message);
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = 8;
    message.text = onOffText[gConfig.musicVolume];
    cPanel->BroadcastMessage(message);
    message.id = 9;
    message.text = onOffText[gConfig.soundVolume];
    cPanel->BroadcastMessage(message);
    message.id = 10;
    message.text = walkSpeedText[gConfig.walkSpeed];
    cPanel->BroadcastMessage(message);
    message.id = 14;
    message.text = musicQualityText[gConfig.musicSource];
    cPanel->BroadcastMessage(message);
    message.id = 15;
    message.text = onOffText[gConfig.showRoute];
    cPanel->BroadcastMessage(message);
    message.id = 16;
    message.text = onOffText[1 - gConfig.blackoutComputer];
    cPanel->BroadcastMessage(message);
    if (!initialDraw)
        cPanel->MoveWindow(0, 0);
}

VA(0x00432bb7, 0x232)
signed char SaveGame(void) {
    short result6;
    fileRequester* requester0;
    char searchMask[16];
    signed char success;
    int humans;
    int i0;
    char extension[8];

    success = 0;
    humans = 0;
    gpAdvManager->DisableButtons();
    gpMouseManager->SetPointer("advmice.mse", 0);
    for (i0 = 0; i0 < 4; i0++)
        if (!gpGame->m_playerDead[i0] && gbHumanPlayer[i0])
            humans++;
    if (giCampaignChoice > 0) {
        sprintf(extension, ".CGM");
        sprintf(searchMask, "*.CGM");
    } else {
        sprintf(extension, ".GM%d", humans);
        sprintf(searchMask, "*.GM*");
    }
    requester0 = new fileRequester(0xa0, 0x28, 1, searchMask, ".\\GAMES\\", extension);
    if (!requester0)
        MemError();
    result6 = gpExec->DoDialog(requester0);
    if (result6 == DIALOG_BUTTON_2) {
        success = 1;
        bFreshSave = 1;
        success = gpGame->SaveGame(gLastFilename, 0);
        if (success)
            NormalDialog("Game saved successfully.", NORMAL_DIALOG_TYPE_OK, 0xb1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, -1);
    }
    delete requester0;
    gpAdvManager->EnableButtons();
    return success;
}

extern char *gCPanelHelp[];

// Buka 2.1 CPanelHandler plus SystemOptionsHandler's option cycling.
VA(0x00432de9, 0x54b)
short CPanelHandler(struct tag_message &message) {
    signed char changed = 0;
    char question[120];
    signed char handled = 0;
    if (message.type == MESSAGE_WIDGET) {
        if (message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON) {
            if (IS_WIDGET_SELECTION_NOTIFICATION(message.command)) {
                int helpIndex = -1;
                switch (message.id) {
                    case CONTROL_NEW_GAME:
                        helpIndex = 0;
                        break;
                    case CONTROL_LOAD_GAME:
                        helpIndex = 1;
                        break;
                    case CONTROL_QUIT:
                        helpIndex = 2;
                        break;
                    case PANEL_CLOSE_WIDGET:
                        helpIndex = 3;
                        break;
                    case CONTROL_SAVE_GAME:
                        helpIndex = 4;
                        break;
                    case CONTROL_MUSIC_VOLUME:
                        helpIndex = 5;
                        break;
                    case CONTROL_SOUND_VOLUME:
                        helpIndex = 6;
                        break;
                    case CONTROL_WALK_SPEED:
                        helpIndex = 7;
                        break;
                    case CONTROL_MUSIC_SOURCE:
                        helpIndex = 8;
                        break;
                    case CONTROL_SHOW_ROUTE:
                        helpIndex = 9;
                        break;
                    case CONTROL_SHOW_ENEMY_MOVES:
                        helpIndex = 10;
                        break;
                    case CONTROL_SCENARIO_INFO:
                        helpIndex = 11;
                        break;
                }
                if (helpIndex >= 0)
                    NormalDialog(gCPanelHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW, 0xb1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, -1);
            }
        } else {
            switch (message.command) {
                case WIDGET_NOTIFY_DESELECT:
                    switch (message.id) {
                        case CONTROL_NEW_GAME:
                            strcpy(question, "Are you sure you want to restart?  (Your current game will be lost)");
                            goto confirm_reset;
                        case CONTROL_LOAD_GAME:
                            strcpy(question, "Are you sure you want to load a new game?  (Your current game will be lost)");
                            goto confirm_reset;
                        case CONTROL_QUIT:
                            strcpy(question, "Are you sure you want to quit?");
                        confirm_reset:
                            handled = 1;
                            if (!bFreshSave) {
                                NormalDialog(question, NORMAL_DIALOG_TYPE_YES_NO, 0xb1, 0x50, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, -1);
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
                            gConfig.musicVolume = (gConfig.musicVolume + 1) % 11;
                            gpSoundManager->AdjustMusicVolumes();
                            changed = 1;
                            bPrefsChanged = 1;
                            break;
                        case CONTROL_SOUND_VOLUME:
                            gConfig.soundVolume = (gConfig.soundVolume + 1) % 11;
                            gpSoundManager->AdjustSoundVolumes();
                            changed = 1;
                            bPrefsChanged = 1;
                            break;
                        case CONTROL_WALK_SPEED:
                            ++gConfig.walkSpeed;
                            gConfig.walkSpeed %= 5;
                            changed = 1;
                            bPrefsChanged = 1;
                            break;
                        case CONTROL_MUSIC_SOURCE:
                            if (gConfig.musicSource == 2) {
                                gConfig.musicSource = 0;
                            } else {
                                if (gpSoundManager->m_cdStarted == 0) {
                                    NormalDialog(
                                        "Unable to set up CD stereo music.  Your CD player might be in use by another "
                                        "program, or your sound driver might not support CD stereo.",
                                        NORMAL_DIALOG_TYPE_OK, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, -1
                                    );
                                    break;
                                }
                                gConfig.musicSource = 2;
                            }
                            gpSoundManager->SetMusicQuality(gConfig.musicSource);
                            changed = 1;
                            bPrefsChanged = 1;
                            break;
                        case CONTROL_SHOW_ROUTE:
                            gConfig.showRoute = 1 - gConfig.showRoute;
                            changed = 1;
                            bPrefsChanged = 1;
                            break;
                        case CONTROL_SHOW_ENEMY_MOVES:
                            if (!gbRemoteOn) {
                                gConfig.blackoutComputer = 1 - gConfig.blackoutComputer;
                                changed = 1;
                                bPrefsChanged = 1;
                            }
                            break;
                    }
                    break;
            }
        }
    }
    if (changed)
        UpdateCPanel(0);
    if (handled) {
        gpWindowManager->m_dialogResult = message.id;
        message.command = message.id = WIDGET_COMMAND_DIALOG_SELECT;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// donor PoL RVA 0x000650eb; preferred Buka symbol ?CheckCastSpell@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.587490;margin=0.208808;shape=0.255;size=0.813;calls=0.857;strings=advmice.mse;alternate=pol20:void advManager::CheckCastSpell(void)@0x000650eb
VA(0x00433334, 0xab)
void advManager::CheckCastSpell(void) {
    if (gpCurPlayer->CurrentHero() != -1) {
        MobilizeCurrHero(0);
        CompleteDraw(0);
        UpdateScreen(0, 0);
        GrabScreen();
        gpMouseManager->SetPointer("advmice.mse", 0);
        CastSpell(
            gpGame->ViewSpells(gpGame->GetHero(gpCurPlayer->m_currentHero), 1, NullHandler, 0)
        );
    }
}

// donor PoL RVA 0x0006a724; preferred Buka symbol ?AdvPanel@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.644374;margin=0.446733;shape=0.427;size=0.786;calls=0.720;strings=advmice.mse|apanel.bin;alternate=pol20:void advManager::AdvPanel(void)@0x0006a724
VA(0x004333df, 0x213)
void advManager::AdvPanel(void)
{
    heroWindow *adventurePanel;
    struct tag_message message;
    int mobilized;

    TrimLoopingSounds(ADVMGR_ACTIVE_SOUND_COUNT);
    gpMouseManager->SetPointer("advmice.mse", 0);
    mobilized = m_heroContextLocked;
    DemobilizeCurrHero();

    adventurePanel = new heroWindow(160, 40, "apanel.bin");
    if (adventurePanel == NULL)
        MemError();
    if (gpCurPlayer->CurrentHero() == -1) {
        message.type = MESSAGE_WIDGET;
        message.id = PANEL_SEARCH;
        message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.value = WIDGET_FLAG_ENABLED;
        adventurePanel->BroadcastMessage(message);
        message.id = PANEL_CAST_SPELL;
        adventurePanel->BroadcastMessage(message);
        message.id = PANEL_SEARCH;
        message.command = WIDGET_COMMAND_SET_FLAGS;
        message.value = WIDGET_COMMAND_DIMMED;
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
            ProcessSearch(-1, -1);
            break;
        case PANEL_VIEW_WORLD:
            ViewWorld(0x18, 0, 0);
            break;
        case PANEL_VIEW_PUZZLE:
            ViewPuzzle();
            break;
    }

    if (mobilized)
        MobilizeCurrHero(0);
}

// donor PoL RVA 0x00065191; preferred Buka symbol ?DimensionDoorHandler@@YIHAAUtag_message@@@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.481967;margin=0.254497;shape=0.313;size=0.765;calls=1.000;alternate=pol20:int DimensionDoorHandler(struct tag_message &)@0x00065191
extern char *gAPanelHelp[];

// Buka 2.1 APanelHandler; HoMM1 shares the search help text with Close and
// chains the dialog-select stores.
VA(0x004335f2, 0x1d3)
short APanelHandler(struct tag_message &message)
{
    signed char handled = 0;
    if (message.type == MESSAGE_WIDGET) {
        if (message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON) {
            if (IS_WIDGET_SELECTION_NOTIFICATION(message.command)) {
                int helpIndex = PANEL_NO_HELP;
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
                        helpIndex = PANEL_SEARCH_HELP;
                        break;
                }
                if (helpIndex >= 0)
                    NormalDialog(gAPanelHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW, 0xb1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, -1);
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
        gpWindowManager->m_dialogResult = message.id;
        message.command = message.id = WIDGET_COMMAND_DIALOG_SELECT;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

VA(0x004337c5, 0x34b)
short DimensionDoorHandler(struct tag_message& message) {
    signed char result;
    short mouseX;
    short mouseY;
    mapCell* cell;

    if (KBTickCount() > glTimers[0]) {
        gpAdvManager->CompleteDraw(gpAdvManager->m_mapOriginX, gpAdvManager->m_mapOriginY, 0);
        gpAdvManager->UpdateScreen(0, 0);
    }
    result = 0;
    switch (message.type) {
        case MESSAGE_WIDGET:
            switch (message.command) {
                case WIDGET_NOTIFY_SELECT:
                    switch (message.id) {
                        case 10:
                        case 11:
                            if (message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON) {
                            } else if (gpWindowManager->m_dialogResult == 1) {
                                result = 1;
                            }
                            break;
                        default:
                            break;
                    }
                    break;
                case WIDGET_COMMAND_HOVER:
                    switch (message.id) {
                        case 11:
                            gpWindowManager->m_dialogResult = 0;
                            gpMouseManager->SetPointer(0);
                            break;
                        case 10:
                            gpMouseManager->MouseCoords(mouseX, mouseY);
                            mouseX /= 32;
                            mouseY /= 32;
                            if (mouseX < 0)
                                mouseX = 0;
                            if (mouseY < 0)
                                mouseY = 0;
                            if (mouseX > 14)
                                mouseX = 14;
                            if (mouseY > 14)
                                mouseY = 14;
                            if (gpAdvManager->m_lastHoverCell != mouseX || gpAdvManager->m_hoverCellY != mouseY) {
                                gpAdvManager->m_lastHoverCell = mouseX;
                                gpAdvManager->m_hoverCellY = mouseY;
                                cell = gpAdvManager->GetCell(
                                    gpAdvManager->m_mapOriginX + mouseX,
                                    gpAdvManager->m_mapOriginY + mouseY
                                );
                                if ((cell->m_triggerType & MAP_TRIGGER_EVENT) || (cell->m_unknown07 & 0x80)) {
                                    gpWindowManager->m_dialogResult = 0;
                                    gpMouseManager->SetPointer(0);
                                } else {
                                    gpWindowManager->m_dialogResult = 1;
                                    gpMouseManager->SetPointer(4);
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
                            gpWindowManager->m_dialogResult = 0;
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

// donor PoL RVA 0x000654ad; preferred Buka symbol ?ComboDraw@advManager@@QAEHHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.384237;margin=0.212683;shape=0.299;size=0.586;calls=0.778;alternate=pol20:int advManager::ComboDraw(int, int, int)@0x000654ad
// HoMM1 retail returns the redraw flag in AL (xor al,al / mov al,1).
VA(0x00433b10, 0xaf6)
signed char advManager::ComboDraw(short originX, short originY, signed char animate) {
    int updateCount;
    int drawY;
    int drawX;
    mapCell* cellPtr;

    PollSound();
    if (!bShowIt)
    return 0;
    if (m_forceCompleteDraw) {
        CompleteDraw(originX, originY, 0);
        return 1;
    }
    if (animate) {
        giFrameCount += giFrameStep;
        if (giFrameCount < 12) {
            Process1WindowsMessage();
            if (KBTickCount() > glTimers[0])
                glTimers[0] = KBTickCount() + 120;
            PollSound();
            return 0;
        } else {
            giFrameCount = 0;
        }
    }

    m_previousOriginX = m_mapOriginX;
    m_previousOriginY = m_mapOriginY;
    memset(bComboDraw, 0, 256);
    m_comboHeroDrawn = 0;

    for (drawX = 0; drawX < 15; drawX++) {
        for (drawY = 0; drawY < 15; drawY++) {
            if (originX + drawX >= 0 && originX + drawX < 72 && originY + drawY >= 0 && originY + drawY < 72) {
                cellPtr = GetCell(originX + drawX, originY + drawY);
                if (cellPtr->m_flags & 0xc)
                    ++bComboDraw[drawX][drawY];
                if (cellPtr->m_triggerType == 0x9a) {
                    ++bComboDraw[drawX][drawY];
                    if (GetCloudLookup(originX + drawX, originY + drawY)) {
                        bComboDraw[drawX + 1][drawY] += 10;
                        if (drawY >= 1) {
                            bComboDraw[drawX][drawY - 1] += 10;
                            bComboDraw[drawX + 1][drawY - 1] += 10;
                        }
                    } else {
                        ++bComboDraw[drawX + 1][drawY];
                        if (drawY >= 1) {
                            ++*(bComboDraw[drawX] + drawY - 1);
                            ++bComboDraw[drawX + 1][drawY - 1];
                        }
                    }
                }
                if (cellPtr->m_triggerType == 0xbd || cellPtr->m_triggerType == 0xbe) {
                    ++bComboDraw[drawX][drawY];
                    if (GetCloudLookup(originX + drawX, originY + drawY)) {
                        bComboDraw[drawX + 1][drawY] += 10;
                        bComboDraw[drawX][drawY + 1] += 10;
                        if (drawY >= 1)
                            bComboDraw[drawX][drawY - 1] += 10;
                        if (drawX >= 1)
                            bComboDraw[drawX - 1][drawY] += 10;
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

    for (drawX = 0; drawX < 15; drawX++) {
        for (drawY = 0; drawY < 15; drawY++) {
            if (bComboDraw[drawX][drawY]) {
                if (originX + drawX < 0 || originX + drawX >= 72 || originY + drawY < 0 || originY + drawY >= 72)
                    bComboDraw[drawX][drawY] = 0;
                else if (bComboDraw[drawX][drawY] < 10 && !GetCloudLookup(originX + drawX, originY + drawY))
                    bComboDraw[drawX][drawY] = 0;
            }
        }
    }

    if (gpMouseManager->IsVis()) {
        drawX = gpMouseManager->m_unknown49 >> 5;
        drawY = gpMouseManager->m_unknown4d >> 5;
        ++bComboDraw[drawX][drawY];
        ++bComboDraw[drawX + 1][drawY];
        ++bComboDraw[drawX][drawY + 1];
        ++bComboDraw[drawX + 1][drawY + 1];
        ++bComboDraw[drawX + 2][drawY + 1];
    }
    if (m_heroContextLocked) {
        for (drawY = 6; drawY <= 8; drawY++)
            for (drawX = 6; drawX <= 8; drawX++)
                ++bComboDraw[drawX][drawY];
    }
    if (m_cursorType == 4) {
        ++bComboDraw[6][5];
        ++bComboDraw[7][5];
        ++bComboDraw[8][5];
    }

    for (drawX = 0; drawX < 15; drawX++) {
        if (bComboDraw[drawX][0])
            DrawCell(originX + drawX, originY, drawX, 0, ADVMGR_DRAW_GROUND | ADVMGR_DRAW_OBJECT, 0, 0);
    }
    for (drawY = 1; drawY < 15; drawY++) {
        for (drawX = 0; drawX < 15; drawX++) {
            if (bComboDraw[drawX][drawY])
                DrawCell(originX + drawX, originY + drawY, drawX, drawY, ADVMGR_DRAW_GROUND, 0, 0);
        }
    }
    for (drawY = 1; drawY < 15; drawY++) {
        PollSound();
        for (drawX = 0; drawX < 15; drawX++) {
            if (bComboDraw[drawX][drawY - 1])
                DrawCell(originX + drawX, originY + drawY - 1, drawX, drawY - 1, ADVMGR_DRAW_OVERLAY | ADVMGR_DRAW_HERO,
                         0, 0);
        }
        for (drawX = 0; drawX < 15; drawX++) {
            if (bComboDraw[drawX][drawY])
                DrawCell(originX + drawX, originY + drawY, drawX, drawY, ADVMGR_DRAW_OBJECT, 0, 0);
        }
    }
    for (drawX = 0; drawX < 15; drawX++) {
        if (bComboDraw[drawX][14])
            DrawCell(originX + drawX, originY + 14, drawX, 14, ADVMGR_DRAW_OVERLAY | ADVMGR_DRAW_HERO, 0, 0);
    }
    for (drawY = 0; drawY < 15; drawY++) {
        for (drawX = 0; drawX < 15; drawX++) {
            if (bComboDraw[drawX][drawY])
                DrawCell(originX + drawX, originY + drawY, drawX, drawY, ADVMGR_DRAW_CLOUD, 0, 0);
        }
    }

    PollSound();
    UpdBottomView(0, 1, 1);
    DrawAdventureBorder();
    giLimitUpdMinX = 15;
    giLimitUpdMinY = 15;
    giLimitUpdMaxX = 0;
    giLimitUpdMaxY = 0;
    updateCount = 0;
    for (drawY = 0; drawY < 15; drawY++) {
        for (drawX = 0; drawX < 15; drawX++) {
            if (bComboDraw[drawX][drawY]) {
                updateCount++;
                if (drawX < giLimitUpdMinX)
                    giLimitUpdMinX = drawX;
                if (drawX > giLimitUpdMaxX)
                    giLimitUpdMaxX = drawX;
                if (drawY < giLimitUpdMinY)
                    giLimitUpdMinY = drawY;
                if (drawY > giLimitUpdMaxY)
                    giLimitUpdMaxY = drawY;
            }
        }
    }
    giLimitUpdMinX <<= 5;
    giLimitUpdMinY <<= 5;
    giLimitUpdMaxX = ((giLimitUpdMaxX + 1) << 5) - 1;
    giLimitUpdMaxY = ((giLimitUpdMaxY + 1) << 5) - 1;
    if (giLimitUpdMinX < 16)
        giLimitUpdMinX = 16;
    if (giLimitUpdMaxX > 463)
        giLimitUpdMaxX = 463;
    if (giLimitUpdMinY < 16)
        giLimitUpdMinY = 16;
    if (giLimitUpdMaxY > 463)
        giLimitUpdMaxY = 463;
    if (giLimitUpdMaxX < giLimitUpdMinX || giLimitUpdMaxY < giLimitUpdMinY) {
        giLimitUpdMinX = giLimitUpdMaxX - 1;
        giLimitUpdMinY = giLimitUpdMaxY - 1;
        return 0;
    }
    return 1;
}

// Buka 2.1 ComboDraw(update) forwards the current map origin.
VA(0x00434606, 0x3a)
signed char advManager::ComboDraw(int update) {
    return ComboDraw(m_mapOriginX, m_mapOriginY, update);
}

// donor PoL RVA 0x0006668e; preferred Buka symbol ?SetEnvironmentOrigin@advManager@@QAEXHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.577153;margin=0.265756;shape=0.425;size=0.947;calls=1.000;alternate=pol20:void advManager::SetEnvironmentOrigin(int, int, int)@0x0006668e
VA(0x00434640, 0x2dd)
void advManager::SetEnvironmentOrigin(short originX, short originY, short stopSounds) {
    int soundRadius;
    int edgeOffset;
    int maxCells = ADVMGR_ACTIVE_SOUND_COUNT / 2;
    int layer;

    if (gpSoundManager->m_musicReady == 0)
        return;
    for (edgeOffset = 0; edgeOffset < ADVMGR_ACTIVE_SOUND_COUNT; ++edgeOffset) {
        if (m_activeSounds[edgeOffset].soundId != -1) {
            if (stopSounds) {
                gpSoundManager->StopSample(
                    m_loopingSamples[m_activeSounds[edgeOffset].soundId]->m_playbackData.activeSample
                );
                m_activeSounds[edgeOffset].soundId = -1;
                m_activeSounds[edgeOffset].volume = 127;
            } else {
                m_activeSounds[edgeOffset].volume = 127;
            }
        }
    }
    if (gConfig.soundVolume != 0) {
        m_activeSoundMask = 0;
        for (layer = 1; layer <= 2; ++layer) {
            InsertSound(originX, originY, 0, layer);
            for (soundRadius = 0; soundRadius < 4; ++soundRadius) {
                for (edgeOffset = 0; edgeOffset < soundRadius * 2; ++edgeOffset) {
                    InsertSound(originX - soundRadius + edgeOffset, originY - soundRadius, soundRadius, layer);
                    InsertSound(originX + soundRadius, originY - soundRadius + edgeOffset, soundRadius, layer);
                    InsertSound(originX + soundRadius - edgeOffset, originY + soundRadius, soundRadius, layer);
                    InsertSound(originX - soundRadius, originY + soundRadius - edgeOffset, soundRadius, layer);
                }
            }
        }
        for (edgeOffset = 0; edgeOffset < ADVMGR_ACTIVE_SOUND_COUNT; ++edgeOffset) {
            if (m_activeSounds[edgeOffset].soundId != -1 && m_activeSounds[edgeOffset].volume > 5) {
                gpSoundManager->StopSample(
                    m_loopingSamples[m_activeSounds[edgeOffset].soundId]->m_playbackData.activeSample
                );
                m_activeSounds[edgeOffset].soundId = -1;
            }
            if (m_activeSounds[edgeOffset].soundId != -1
                && (m_activeSoundMask & (1 << m_activeSounds[edgeOffset].soundId)) != 0) {
                gpSoundManager->ModifySample(
                    m_loopingSamples[m_activeSounds[edgeOffset].soundId]->m_playbackData.activeSample,
                    100,
                    glEnvironmentVolume[m_activeSounds[edgeOffset].volume]
                );
            }
        }
    }
}

// donor PoL RVA 0x000669c6; preferred Buka symbol ?CheckLoadSample@advManager@@QAEXH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:1;base=0.741786;margin=0.490066;shape=0.533;size=0.857;calls=1.000;strings=loop%04d.82M;alternate=pol20:void advManager::CheckLoadSample(int)@0x000669c6
VA(0x0043491d, 0x69)
void advManager::CheckLoadSample(int index) {
    if (m_loopingSamples[index] == NULL) {
        TrimLoopingSounds(ADVMGR_ACTIVE_SOUND_COUNT);
        sprintf(gText, "loop%04d.82M", index);
        m_loopingSamples[index] = gpResourceManager->GetSample(gText);
    }
}

// donor PoL RVA 0x00066ef0; preferred Buka symbol ?InsertSound@advManager@@QAEXHHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.476286;margin=0.526376;shape=0.266;size=0.902;calls=0.750;alternate=pol20:void advManager::InsertSound(int, int, int, int)@0x00066ef0
VA(0x00434986, 0x251)
void advManager::InsertSound(short x, short y, short distance, signed char soundLayer) {
    int slot;
    int distanceLimit;
    int i;
    int soundId;

    if (x < 0 || y < 0 || x >= 72 || y >= 72)
        return;
    soundId = gpGame->m_mapSounds[x][y];
    if (soundId == -1)
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
    if (soundLayer == 1)
        return;
    distanceLimit = distance;
    slot = -1;
    for (i = 0; i < ADVMGR_ACTIVE_SOUND_COUNT; ++i) {
        if (m_activeSounds[i].volume > distanceLimit) {
            distanceLimit = m_activeSounds[i].volume;
            slot = i;
        }
    }
    if (slot != -1) {
        if (m_activeSounds[slot].soundId != -1)
            gpSoundManager->StopSample(m_loopingSamples[m_activeSounds[slot].soundId]->m_playbackData.activeSample);
        m_activeSounds[slot].soundId = soundId;
        m_activeSounds[slot].volume = distance;
        CheckLoadSample(soundId);
        m_loopingSamples[soundId]->m_playbackData.volume = glEnvironmentVolume[distance];
        m_loopingSamples[soundId]->m_playbackData.loopCount = 0;
        m_loopingSamples[soundId]->m_playbackData.channelType = 3;
        gpSoundManager->MemorySample(m_loopingSamples[soundId]);
        m_activeSoundMask ^= 1 << m_activeSounds[slot].soundId;
    }
}

// donor PoL RVA 0x0006712a; preferred Buka symbol ?TeleportTo@advManager@@QAEXPAVhero@@HHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.494469;margin=0.364782;shape=0.352;size=0.864;calls=0.864;alternate=pol20:void advManager::TeleportTo(class hero *, int, int, int, int)@0x0006712a
VA(0x00434bd7, 0x340)
void advManager::TeleportTo(int x, int y, int) {
    int savedShow;
    int fizzle;
    mapCell* destinationCell;
    mapCell* oldCell;
    int tmp;
    signed char newTerrain;
    hero* mapHero;
    town* occupiedTown;

    savedShow = bShowIt;
    mapHero = gpGame->GetHero(gpCurPlayer->m_currentHero);
    destinationCell = GetCell(x, y);
    oldCell = GetCell(m_mapOriginX + 7, m_mapOriginY + 7);
    if (mapHero->m_locationType == 0xa8) {
        occupiedTown = gpGame->GetTown(mapHero->m_occupiedTown);
        occupiedTown->m_occupyingHeroId = -1;
    }
    if (oldCell->m_flags & 0x40)
        oldCell->m_flags -= 0x40;
    CompleteDraw(0);
    if (!gbHumanPlayer[giCurPlayer]) {
        if (!gConfig.blackoutComputer && !gbRemoteOn
            && (gpGame->m_mapExtra[mapHero->m_x][mapHero->m_y] & giCurPlayerBit))
            bShowIt = 1;
        else
            bShowIt = 0;
    }
    if (savedShow)
        HideRoute(1, 1, 1);
    if (bShowIt) {
        m_mapOriginX = x - 7;
        m_mapOriginY = y - 7;
        DelayMilli(90);
    }
    mapHero->m_x = x;
    mapHero->m_y = y;
    gpGame->SetVisibility(
        m_mapOriginX + 7,
        m_mapOriginY + 7,
        giCurPlayer,
        gHeroScoutRadius[mapHero->m_unknown1c]
    );
    if (bShowIt) {
        destinationCell->m_flags |= 0x40;
        gpMouseManager->ReallyHidePointer();
        gpWindowManager->SaveFizzleSource(16, 16, 448, 448);
        CompleteDraw(0);
        PollSound();
        fizzle = 128;
        if (!gbHumanPlayer[giCurPlayer])
            fizzle -= 64;
        gpWindowManager->FizzleForward(16, 16, 448, 448, -1);
        PollSound();
        gpMouseManager->ReallyShowPointer();
    }
    SetEnvironmentOrigin(m_mapOriginX + 7, m_mapOriginY + 7, 1);
    newTerrain = giGroundToTerrain[destinationCell->m_tileIndex];
    if (m_currentTerrain != newTerrain) {
        m_currentTerrain = newTerrain;
        gpSoundManager->SwitchAmbientMusic(m_currentTerrain);
    }
    Reseed(0, 0);
    UpdateRadar(1, 0);
    CompleteDraw(0);
    ForceNewHover();
}

// donor PoL RVA 0x00067539; preferred Buka symbol ?DimensionDoor@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.671113;margin=0.501597;shape=0.372;size=0.883;calls=0.867;strings=dimdoor.bin;alternate=pol20:void advManager::DimensionDoor(void)@0x00067539
VA(0x00434f17, 0x246)
void advManager::DimensionDoor(void) {
    hero* heroPointer;
    heroWindow* win;
    short x;
    short y;
    mapCell* targetCell;

    win = new heroWindow(0, 0, "dimdoor.bin");
    if (win == NULL)
        MemError();
    SetWinText(win, 4);
    gpWindowManager->DoDialog(win, DimensionDoorHandler, 0);
    delete win;
    heroPointer = gpGame->GetHero(gpCurPlayer->m_currentHero);
    if (gpWindowManager->m_dialogResult == 1) {
        x = m_mapOriginX + m_lastHoverCell;
        y = m_mapOriginY + m_hoverCellY;
        targetCell = GetCell(x, y);
        if (((heroPointer->m_eventFlags & HERO_EVENT_EMBARKED) && targetCell->m_tileIndex >= 20)
            || (!(heroPointer->m_eventFlags & HERO_EVENT_EMBARKED) && targetCell->m_tileIndex < 20)) {
            NormalDialog("Dimension Door failed!!!", NORMAL_DIALOG_TYPE_OK, 0x61, 0x91, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, -1);
            UpdateRadar(1, 0);
        } else {
            gpSoundManager->SwitchAmbientMusic(16);
            TeleportTo(x, y, 0);
            gpSoundManager->SwitchAmbientMusic(m_currentTerrain);
        }
        gpGame->GetHero(gpCurPlayer->m_currentHero)->UseSpell(SPELL_DIMENSION_DOOR);
    } else {
        UpdateRadar(1, 0);
    }
}

// donor PoL RVA 0x0006785d; preferred Buka symbol ?TownGate@advManager@@QAEXH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.385467;margin=0.208066;shape=0.290;size=0.657;calls=0.526;alternate=pol20:void advManager::TownGate(int)@0x0006785d
VA(0x0043515d, 0x2a6)
void advManager::TownGate(void) {
    int k;
    int bestDist;
    int bestTown;
    hero* heroPointer;
    int distance;

    bestDist = 1000;
    bestTown = -1;
    heroPointer = gpGame->GetHero(gpCurPlayer->m_currentHero);
    if (heroPointer->m_eventFlags & HERO_EVENT_EMBARKED) {
        NormalDialog(
            "Town Gate Failed!!!  You must be on land for this spell to work.",
            NORMAL_DIALOG_TYPE_OK, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, -1
        );
        return;
    }
    for (k = 0; k < gpCurPlayer->m_townCount; k++) {
        distance = abs(gpGame->m_castleRecs[gpCurPlayer->m_townIds[k]].m_x - heroPointer->m_x)
                   + abs(gpGame->m_castleRecs[gpCurPlayer->m_townIds[k]].m_y - heroPointer->m_y);
        if (distance < bestDist) {
            bestDist = distance;
            bestTown = k;
        }
    }
    if (bestTown == -1)
        NormalDialog("No available town.  Town Gate Failed!!!", NORMAL_DIALOG_TYPE_OK, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, -1);
    if (gpGame->m_castleRecs[gpCurPlayer->m_townIds[bestTown]].m_occupyingHeroId != -1) {
        NormalDialog("Nearest town occupied.  Town Gate Failed!!!", NORMAL_DIALOG_TYPE_OK, 0x61, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, -1);
        return;
    }
    gpSoundManager->SwitchAmbientMusic(16);
    TeleportTo(
        gpGame->m_castleRecs[gpCurPlayer->m_townIds[bestTown]].m_x,
        gpGame->m_castleRecs[gpCurPlayer->m_townIds[bestTown]].m_y,
        0
    );
    heroPointer->UseSpell(SPELL_TOWN_GATE);
    gpGame->m_castleRecs[gpCurPlayer->m_townIds[bestTown]].m_occupyingHeroId = heroPointer->m_id;
    gpGame->m_castleRecs[gpCurPlayer->m_townIds[bestTown]].GiveSpells();
    heroPointer->m_locationType = (MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN);
    heroPointer->m_occupiedTown = gpCurPlayer->m_townIds[bestTown];
    gpSoundManager->SwitchAmbientMusic(m_currentTerrain);
}

// donor PoL RVA 0x00067c9b; preferred Buka symbol ?SummonBoat@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.502597;margin=0.051626;shape=0.294;size=0.965;calls=0.867;alternate=pol20:void advManager::SummonBoat(void)@0x00067c9b
VA(0x00435403, 0x51c)
void advManager::SummonBoat(void) {
    hero* pHero;
    signed char boatFound;
    mapCell* pCell;
    short iDirection;
    signed char foundCell;
    boatRecord* thisBoat;
    short slotIndex;
    signed char heroNum;
    mapCell* fromCell;
    short drawX;
    short drawY;
    short drawHeight;
    short drawWidth;

    pHero = &gpGame->m_heroRecs[gpCurPlayer->CurrentHero()];
    foundCell = 0;
    boatFound = 0;
    pCell = GetCell(m_mapOriginX + 7, m_mapOriginY + 7);
    if (pCell->m_tileIndex < 20)
        goto summon_done;
    for (iDirection = 0; iDirection < 8; iDirection++) {
        pCell = GetCell(normalDirTable[iDirection].x + m_mapOriginX + 7,
                                  normalDirTable[iDirection].y + m_mapOriginY + 7);
        if (pCell->m_objectIndex == 0xff && pCell->m_tileIndex < 20) {
            foundCell = 1;
            break;
        }
    }
    if (foundCell) {
        heroNum = gpCurPlayer->CurrentHero();
        for (slotIndex = 0; slotIndex < GAME_BOAT_COUNT; slotIndex++) {
            if (gpGame->m_boatSlots[slotIndex] != -1 && gpGame->m_boats[slotIndex].heroId == (heroNum | 0x80)) {
                boatFound = 1;
                break;
            }
        }
        if (!boatFound) {
            for (slotIndex = 0; slotIndex < GAME_BOAT_COUNT; slotIndex++) {
                if (gpGame->m_boatSlots[slotIndex] != -1 && (gpGame->m_boats[slotIndex].heroId & 0x80)
                    && gpGame->m_boats[slotIndex].owner == giCurPlayer) {
                    boatFound = 1;
                    break;
                }
            }
        }
        if (boatFound) {
            thisBoat = &gpGame->m_boats[slotIndex];
            fromCell = GetCell(thisBoat->x, thisBoat->y);
            gpGame->RestoreCell(thisBoat->x, thisBoat->y, thisBoat->savedTriggerType, thisBoat->savedEventData, 0, 5);
            if (thisBoat->x >= m_mapOriginX && thisBoat->x < m_mapOriginX + 15 && thisBoat->y >= m_mapOriginY
                && thisBoat->y < m_mapOriginY + 15) {
                drawX = (thisBoat->x - m_mapOriginX) * 32 - 32;
                if (drawX < 16)
                    drawX = 16;
                drawY = (thisBoat->y - m_mapOriginY) * 32 - 16;
                if (drawY < 16)
                    drawY = 16;
                drawWidth = 96;
                drawHeight = 48;
                if (drawX + drawWidth >= 464)
                    drawWidth = 464 - drawX;
                if (drawY + drawHeight >= 464)
                    drawHeight = 464 - drawY;
                gpWindowManager->SaveFizzleSource(drawX, drawY, drawWidth, drawHeight);
                CompleteDraw(m_mapOriginX, m_mapOriginY, 0);
                gpWindowManager->FizzleForward(drawX, drawY, drawWidth, drawHeight, -1);
            }
            thisBoat->x = normalDirTable[iDirection].x + m_mapOriginX + 7;
            thisBoat->y = normalDirTable[iDirection].y + m_mapOriginY + 7;
            thisBoat->savedTriggerType = pCell->m_triggerType;
            thisBoat->savedEventData = pCell->m_objectMetadata;
            pCell->m_triggerType = 0xbe;
            pCell->m_objectMetadata = slotIndex;
            gpWindowManager->SaveFizzleSource(176, 192, 128, 96);
            CompleteDraw(m_mapOriginX, m_mapOriginY, 0);
            gpWindowManager->FizzleForward(176, 192, 128, 96, -1);
        }
    }

summon_done:
    UpdateScreen(0, 0);
    Reseed(0, 0);
    if (!boatFound)
        NormalDialog("Summon Boat failed!!!", 1, 0x61, 0x91, -1, 0, -1, 0, -1);
}

// donor PoL RVA 0x00068247; preferred Buka symbol ?ShowRoute@advManager@@QAEXHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.438878;margin=0.464254;shape=0.279;size=0.721;calls=1.000;alternate=pol20:void advManager::ShowRoute(int, int, int)@0x00068247
VA(0x0043591f, 0x31b)
void advManager::ShowRoute(int redraw, int, int updateButton) {
    hero* pHero;
    int canReach;
    int fromDirection;
    int x;
    int y;
    int dir;
    int j;
    int remMob;
    int terr;
    short buttonFrame;

    canReach = 0;
    if (!gbThisNetHumanPlayer[giCurPlayer] && (!giDebugLevel || !giShowComputerRoute))
        return;
    if (gpCurPlayer->m_currentHero == -1) {
        HideRoute(redraw, 0, 1);
        return;
    }
    pHero = gpGame->GetHero(gpCurPlayer->m_currentHero);
    if (pHero->m_destinationX == -1) {
        HideRoute(redraw, 1, 1);
        return;
    }
    gpSearchArray->BuildPath(pHero->m_x, pHero->m_y, pHero->m_destinationX, pHero->m_destinationY, 999);
    if (gpSearchArray->m_pathLength > 0) {
        memset(m_visibilityMap, 0, MAP_CELL_GRID_SIZE * MAP_CELL_GRID_SIZE);
        m_routeShown = 1;
        remMob = pHero->m_remainingMobility;
        x = pHero->m_x;
        y = pHero->m_y;
        for (j = gpSearchArray->m_pathLength - 1; j >= 0; --j) {
            dir = gpSearchArray->m_directions[j];
            terr = giGroundToTerrain[GetCell(x, y)->m_tileIndex];
            remMob -= CalcTerrainCost(terr, dir & 1, remMob, pHero->m_unknown1c);
            x += normalDirTable[dir].x;
            y += normalDirTable[dir].y;
            if (j == 0) {
                m_visibilityMap[y * MAP_CELL_GRID_SIZE + x] = 14;
            } else {
                fromDirection = gpSearchArray->m_directions[j - 1];
                m_visibilityMap[y * MAP_CELL_GRID_SIZE + x] = gRouteFrame[fromDirection][dir];
            }
            if (remMob >= 0) {
                m_visibilityMap[y * MAP_CELL_GRID_SIZE + x] = m_visibilityMap[y * MAP_CELL_GRID_SIZE + x] + 14;
                canReach = 1;
            }
        }
        if (updateButton) {
            buttonFrame = canReach ? 6 : 5;
            gpWindowManager->BroadcastMessage(MESSAGE_WIDGET, buttonFrame, 2, 0x4008);
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

// donor PoL RVA 0x00068720; preferred Buka symbol ?HideRoute@advManager@@QAEXHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.524051;margin=0.948488;shape=0.403;size=0.809;calls=1.000;alternate=pol20:void advManager::HideRoute(int, int, int)@0x00068720
VA(0x00435c3a, 0x106)
void advManager::HideRoute(int redraw, int clearDestination, int updateButton) {
    hero* currentHero;

    if (!gbThisNetHumanPlayer[giCurPlayer] && (!giDebugLevel || !giShowComputerRoute))
        return;

    if (updateButton)
        gpWindowManager->BroadcastMessage(
            MESSAGE_WIDGET,
            WIDGET_COMMAND_SET_FLAGS,
            PANEL_CONTINUE_ROUTE,
            WIDGET_FLAG_UPDATE | WIDGET_FLAG_DIMMED
        );

    if (clearDestination && gpCurPlayer->m_currentHero != -1) {
        currentHero = gpGame->GetHero(gpCurPlayer->m_currentHero);
        currentHero->m_destinationX = -1;
        currentHero->m_destinationY = -1;
    }

    if (!m_routeShown)
        return;

    m_routeShown = 0;
    if (redraw) {
        CompleteDraw(0);
        UpdateScreen(0, 0);
    }
}

// donor PoL RVA 0x00068827; preferred Buka symbol ?CheckDimHero@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.465466;margin=0.233620;shape=0.306;size=0.738;calls=1.000;alternate=pol20:void advManager::CheckDimHero(void)@0x00068827
VA(0x00435d40, 0x91)
void advManager::CheckDimHero(void) {
    if (!gbThisNetHumanPlayer[giCurPlayer] || gpCurPlayer->CurrentHero() == -1)
        return;
    if (!gpGame->IsMobile(gpCurPlayer->CurrentHero())) {
        ShowRoute(1, 0, 0);
        UpdateHeroLocators(1, 1);
        gpAdvManager->CheckDimNextHeroBut();
    }
}

// donor PoL RVA 0x000688b4; preferred Buka symbol ?CheckDimNextHeroBut@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.454682;margin=0.437763;shape=0.225;size=0.845;calls=1.000;alternate=pol20:void advManager::CheckDimNextHeroBut(void)@0x000688b4
VA(0x00435dd1, 0x6e)
void advManager::CheckDimNextHeroBut(void) {
    short frame;

    if (!gbThisNetHumanPlayer[giCurPlayer] || !gpCurPlayer->HasMobileHero())
        frame = WIDGET_COMMAND_SET_FLAGS;
    else
        frame = WIDGET_COMMAND_CLEAR_FLAGS;
    gpWindowManager->BroadcastMessage(
        MESSAGE_WIDGET,
        frame,
        BUTTON_BROADCAST_ARG,
        WIDGET_FLAG_UPDATE | WIDGET_FLAG_DIMMED
    );
}

// donor PoL RVA 0x0006891f; preferred Buka symbol ?SeedTo@advManager@@QAEXHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.530039;margin=0.750383;shape=0.400;size=0.820;calls=1.000;alternate=pol20:void advManager::SeedTo(int, int)@0x0006891f
VA(0x00435e3f, 0x152)
void advManager::SeedTo(int targetX, int targetY)
{
    hero *currentHero;

    if (!gbThisNetHumanPlayer[giCurPlayer])
        return;
    if (gpCurPlayer->m_currentHero == -1)
        return;

    currentHero = gpGame->GetHero(gpCurPlayer->m_currentHero);
    if (!giSeedingValid)
        gpSearchArray->SeedPosition(currentHero->m_x, currentHero->m_y, m_cursorDirection, 999,
                                    m_cursorType == 4, 0, currentHero->m_remainingMobility,
                                    currentHero->m_unknown1c, targetX, targetY, 0, 1);
    else if (!giFullySeeded)
        gpSearchArray->SeedPosition(currentHero->m_x, currentHero->m_y, m_cursorDirection, 999,
                                    m_cursorType == 4, 0, currentHero->m_remainingMobility,
                                    currentHero->m_unknown1c, targetX, targetY, 1, 1);
}

// donor PoL RVA 0x00068ab6; preferred Buka symbol ?ScreenScroll@advManager@@QAEXHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.463708;margin=0.069995;shape=0.184;size=0.968;calls=1.000;alternate=pol20:void advManager::ScreenScroll(int, int)@0x00068ab6
// Buka 2.1 ForceNewHover; HoMM1 routes the hover through a message record.
VA(0x00435f91, 0x4f)
void advManager::ForceNewHover(void) {
    struct tag_message msg;

    if (!gbThisNetHumanPlayer[giCurPlayer])
        return;
    m_lastHoverCell = -1;
    msg.id = 10;
    ProcessHover(&msg);
}

VA(0x00435fe0, 0x1b6)
void advManager::ScreenScroll(signed char direction, int updatePointer)
{
    short yOrigin;
    short xOrigin;

    xOrigin = m_mapOriginX;
    yOrigin = m_mapOriginY;
    iLastScrollTime = KBTickCount();

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

// donor PoL RVA 0x00068c5c; preferred Buka symbol ?CheckScreenScroll@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.475412;margin=0.607960;shape=0.306;size=0.761;calls=1.000;alternate=pol20:void advManager::CheckScreenScroll(void)@0x00068c5c
VA(0x00436196, 0x1e1)
void advManager::CheckScreenScroll(void)
{
    short mouseX;
    short mouseY;
    int oldX;
    int oldY;

    if (KBTickCount() - iLastScrollTime > SCROLL_TICK_INTERVAL) {
        iLastScrollTime = KBTickCount();
        oldX = m_mapOriginX;
        oldY = m_mapOriginY;
        gpMouseManager->MouseCoords(mouseX, mouseY);

        if (mouseX >= 0 && mouseX < LOGICAL_SCREEN_WIDTH && mouseY >= 0 && mouseY < LOGICAL_SCREEN_HEIGHT) {
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
            && gpMouseManager->m_cursorFrame < HOVER_SCROLL_FRAME_END && oldX == m_mapOriginX
            && oldY == m_mapOriginY)
            gpMouseManager->SetPointer(0);
    }
}

// donor PoL RVA 0x00068e17; preferred Buka symbol ?MouseInScrollZone@advManager@@QAEHXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.376012;margin=0.441568;shape=0.180;size=0.620;calls=1.000;alternate=pol20:int advManager::MouseInScrollZone(void)@0x00068e17
VA(0x00436377, 0xa3)
int advManager::MouseInScrollZone(void) {
    short mouseX;
    short mouseY;

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

// donor PoL RVA 0x00068ea8; preferred Buka symbol ?SetInitialMapOrigin@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.512925;margin=0.905583;shape=0.330;size=0.958;calls=0.778;alternate=pol20:void advManager::SetInitialMapOrigin(void)@0x00068ea8
VA(0x0043641a, 0x283)
void advManager::SetInitialMapOrigin(void) {
    short x;
    short y;
    game* gameState;
    hero* initialHero;
    town* townPointer;
    town* ownTown;

    gpWindowManager->BroadcastMessage(MESSAGE_WIDGET, WIDGET_COMMAND_SET_FLAGS, 2, 0x4008);
    m_lastHoverCell = m_hoverCellY = 0;
    m_cursorActive = 0;
    gbHeroMoving = 0;
    if (gpCurPlayer->CurrentTown() != -1) {
        townPointer = gpGame->GetTown(gpCurPlayer->m_currentTown);
        m_mapOriginX = townPointer->m_x - 7;
        m_mapOriginY = townPointer->m_y - 7;
    } else if (gpCurPlayer->CurrentHero() != -1) {
        MobilizeCurrHero(0);
    } else if (gpCurPlayer->m_heroCount > 0) {
        initialHero = &gpGame->m_heroRecs[gpCurPlayer->m_heroIds[0]];
        m_mapOriginX = initialHero->m_x - 7;
        m_mapOriginY = initialHero->m_y - 7;
    } else if (gpCurPlayer->m_townCount > 0) {
        ownTown = &gpGame->m_castleRecs[gpCurPlayer->m_townIds[0]];
        m_mapOriginX = ownTown->m_x - 7;
        m_mapOriginY = ownTown->m_y - 7;
    } else {
        m_mapOriginX = 0;
        m_mapOriginY = 0;
    }
    m_currentTerrain = giGroundToTerrain[GetCell(m_mapOriginX + 7, m_mapOriginY + 7)->m_tileIndex];
    gpSoundManager->SwitchAmbientMusic(m_currentTerrain);
    SetEnvironmentOrigin(m_mapOriginX + 7, m_mapOriginY + 7, 1);
    gpMouseManager->MouseCoords(x, y);
    gpMouseManager->WarpPointer(x - 20, y - 20);
    Reseed(0, 0);
    CheckDimNextHeroBut();
}

// donor PoL RVA 0x00069160; preferred Buka symbol ?LoadRemote@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.559158;margin=0.708697;shape=0.333;size=0.638;calls=0.706;strings=advmice.mse;alternate=pol20:void advManager::LoadRemote(void)@0x00069160
VA(0x0043669d, 0x152)
void advManager::LoadRemote(void)
{
    gpMouseManager->ReallyHidePointer();
    if (gbThisNetHumanPlayer[giCurPlayer])
        gpMouseManager->SetPointer("advmice.mse", 0);
    gpGame->LoadGame("REMOTE.GAM", 0, 1);
    if (gbThisNetHumanPlayer[giCurPlayer])
        gpGame->CancelComputerScreen();
    gpGame->DoNewTurn();
    UpdateHeroLocators(1, 1);
    UpdateTownLocators(1, 1);
    UpdateRadar(1, 0);
    gpMouseManager->ReallyShowPointer();
    UpdBottomView(1, 1, 1);
    if ((gpGame->m_day != 1 || (gpGame->m_week == 1 && gpGame->m_month == 1)) && gbRemoteOn
        && gbThisNetHumanPlayer[giCurPlayer] && giForceSwitchMusic == -1) {
        gpSoundManager->SwitchAmbientMusic(15);
        giForceSwitchMusic = KBTickCount();
    }
}

// donor PoL RVA 0x0006931e; preferred Buka symbol ?CheckHandleNet@advManager@@QAEPADXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.410587;margin=0.478746;shape=0.288;size=0.750;calls=0.692;alternate=pol20:char * advManager::CheckHandleNet(void)@0x0006931e
VA(0x004367ef, 0x178)
char* advManager::CheckHandleNet(void) {
    RemoteMessage* receivedPacket;
    int remotePlayerExited;

    receivedPacket = reinterpret_cast<RemoteMessage*>(GetRemoteData(1));
    if (receivedPacket && receivedPacket->type == REMOTE_MESSAGE_RELIABLE) {
        switch (receivedPacket->command) {
        case 1:
            remotePlayerExited = receivedPacket->payload.playerExited;
            if (!gpGame->ReceiveSaveGame(receivedPacket->payload.saveSize, receivedPacket->sender))
                ShutDown(NULL);
            if (remotePlayerExited)
                ReceiveRemotePlayerExit(receivedPacket->sender, 0, 1, 0);
            LoadRemote();
            break;
        case 11:
            PopNetBox(receivedPacket->payload.data);
            break;
        case 21:
            if (gbInCombat)
                return reinterpret_cast<char*>(receivedPacket);
            else
                DoNetCombat(reinterpret_cast<char*>(receivedPacket));
            break;
        case 30:
            LogStr("receive exit");
            ReceiveRemotePlayerExit(receivedPacket->payload.data[0], receivedPacket->payload.data[1], 0, 0);
            break;
        default:
            return reinterpret_cast<char*>(receivedPacket);
        }
    }
    return NULL;
}

// donor PoL RVA 0x0006952a; preferred Buka symbol ?CheckHandleNetPlayerWait@advManager@@QAEHAAUtag_message@@H@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.447497;margin=1.157842;shape=0.264;size=0.731;calls=1.000;alternate=pol20:int advManager::CheckHandleNetPlayerWait(struct tag_message &, int)@0x0006952a
VA(0x00436967, 0xd4)
short advManager::CheckHandleNetPlayerWait(struct tag_message &message, signed char doMain)
{
    if (message.type == MESSAGE_MOUSE_MOVE)
        gpMouseManager->Main(message);

    CheckDoMain(1, doMain);
    if (message.type == MESSAGE_KEY_DOWN) {
        switch (message.keyCode) {
            case INPUT_SCAN_F1:
                PopNetBox(NULL);
                break;

            case INPUT_SCAN_Q:
                if (message.modifiers & 0xc) {
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

// donor PoL RVA 0x000695f7; preferred Buka symbol ?TrimLoopingSounds@advManager@@QAEXH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.593881;margin=0.540898;shape=0.483;size=0.978;calls=1.000;alternate=pol20:void advManager::TrimLoopingSounds(int)@0x000695f7
VA(0x00436a3b, 0x1c2)
void advManager::TrimLoopingSounds(int maxSamples)
{
    if (giHighMemBuffer > 0)
        maxSamples += giHighMemBuffer / 100;

    if (maxSamples >= ADVMGR_ENVIRONMENT_SOUND_COUNT)
        return;

    signed char keep[ADVMGR_ENVIRONMENT_SOUND_COUNT];
    int loaded = 0;
    memset(keep, 0, sizeof(keep));

    int i;
    for (i = 0; i < ADVMGR_ACTIVE_SOUND_COUNT; ++i) {
        if (m_activeSounds[i].soundId >= 0 && m_activeSounds[i].soundId < ADVMGR_ENVIRONMENT_SOUND_COUNT)
            ++keep[m_activeSounds[i].soundId];
    }

    for (i = 0; i < ADVMGR_ENVIRONMENT_SOUND_COUNT; ++i) {
        if (keep[i] != 0)
            ++loaded;
    }

    if (loaded < maxSamples) {
        for (i = 0; i < ADVMGR_ENVIRONMENT_SOUND_COUNT; ++i) {
            if (keep[i] == 0 && m_loopingSamples[i] != 0) {
                ++keep[i];
                ++loaded;
                if (loaded >= maxSamples)
                    goto disposeSamples;
            }
        }
    }

disposeSamples:
    for (i = 0; i < ADVMGR_ENVIRONMENT_SOUND_COUNT; ++i) {
        if (m_loopingSamples[i] != 0 && keep[i] == 0) {
            gpResourceManager->Dispose(m_loopingSamples[i]);
            m_loopingSamples[i] = 0;
        }
    }
}

// donor PoL RVA 0x00069976; preferred Buka symbol ?SaveAdventureBorder@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.567475;margin=0.473800;shape=0.423;size=0.969;calls=1.000;alternate=pol20:void advManager::SaveAdventureBorder(void)@0x00069976
VA(0x00436bfd, 0xd0)
void advManager::DisableButtons(void) {
    if (gpAdvManager->m_active != 1)
        return;
    struct tag_message message;
    SET_ADVENTURE_BUTTON_FLAGS(message, m_adventureWindow, WIDGET_COMMAND_CLEAR_FLAGS);
}

VA(0x00436ccd, 0xd0)
void advManager::EnableButtons(void) {
    if (gpAdvManager->m_active != 1)
        return;
    struct tag_message message;
    SET_ADVENTURE_BUTTON_FLAGS(message, m_adventureWindow, WIDGET_COMMAND_SET_FLAGS);
}

VA(0x00436d9d, 0x138)
void advManager::SaveAdventureBorder(void) {
    if (m_adventureBorder != NULL)
        return;

    m_adventureBorder = static_cast<unsigned char*>(malloc(BORDER_BUFFER_SIZE));
    unsigned char* savedPixels = m_adventureBorder;
    unsigned char* src = reinterpret_cast<unsigned char*>(gpWindowManager->m_screen->m_pixels);
    int row;
    for (row = 0; row < BORDER_EDGE_SIZE; ++row) {
        memcpy(savedPixels, src, ADVENTURE_VIEWPORT_EXTENT);
        src += LOGICAL_SCREEN_WIDTH;
        savedPixels += ADVENTURE_VIEWPORT_EXTENT;
    }
    for (row = BORDER_EDGE_SIZE; row < BORDER_MIDDLE_END; ++row) {
        memcpy(savedPixels, src, BORDER_SIDE_BYTES);
        memcpy(savedPixels + BORDER_SIDE_BYTES, src + BORDER_MIDDLE_END, BORDER_SIDE_BYTES);
        src += LOGICAL_SCREEN_WIDTH;
        savedPixels += BORDER_SAVED_SIDE_BYTES;
    }
    for (row = BORDER_MIDDLE_END; row < LOGICAL_SCREEN_HEIGHT; ++row) {
        memcpy(savedPixels, src, ADVENTURE_VIEWPORT_EXTENT);
        src += LOGICAL_SCREEN_WIDTH;
        savedPixels += ADVENTURE_VIEWPORT_EXTENT;
    }
}

// donor PoL RVA 0x00069abb; preferred Buka symbol ?DrawAdventureBorder@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.567997;margin=0.477634;shape=0.397;size=0.978;calls=1.000;alternate=pol20:void advManager::DrawAdventureBorder(void)@0x00069abb
VA(0x00436ed5, 0x134)
void advManager::DrawAdventureBorder(void) {
    unsigned char* savedPixels;
    unsigned char* dest;
    int row;

    if (m_adventureBorder == NULL)
        return;
    if (gbNoBorder != 0)
        return;

    dest = reinterpret_cast<unsigned char*>(gpWindowManager->m_screen->m_pixels);
    savedPixels = m_adventureBorder;
    for (row = 0; row < BORDER_EDGE_SIZE; ++row) {
        memcpy(dest, savedPixels, ADVENTURE_VIEWPORT_EXTENT);
        dest += LOGICAL_SCREEN_WIDTH;
        savedPixels += ADVENTURE_VIEWPORT_EXTENT;
    }
    for (row = BORDER_EDGE_SIZE; row < BORDER_MIDDLE_END; ++row) {
        memcpy(dest, savedPixels, BORDER_SIDE_BYTES);
        memcpy(dest + BORDER_MIDDLE_END, savedPixels + BORDER_SIDE_BYTES, BORDER_SIDE_BYTES);
        dest += LOGICAL_SCREEN_WIDTH;
        savedPixels += BORDER_SAVED_SIDE_BYTES;
    }
    for (row = BORDER_MIDDLE_END; row < LOGICAL_SCREEN_HEIGHT; ++row) {
        memcpy(dest, savedPixels, ADVENTURE_VIEWPORT_EXTENT);
        dest += LOGICAL_SCREEN_WIDTH;
        savedPixels += ADVENTURE_VIEWPORT_EXTENT;
    }
}

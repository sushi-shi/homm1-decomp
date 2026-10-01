// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <BASE/INPUTMGR_TYPES.h>
#include <BASE/Icon2b.h>
#include <BASE/MISC_TYPES.h>
#include <BASE/Misc.h>
#include <BASE/bmap2.h>
#include <H1/All.h>
#include <H1/KB.h>
#include <SOURCE/highScoreRuntime.h>
#include <SOURCE/wingraph.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Buka's giSeedingValid is the dword zeroed by retail Reseed at VA 0x4c5170.
// Code-use identity only; no initializer-byte coverage is asserted.
DATA(0x004c5170)
int giSeedingValid;

// clang-format off
H1_ENUM_BEGIN(AdventureButtonConstant)
    BUTTON_BROADCAST_ARG = 1,
    PANEL_CONTINUE_ROUTE = 2
H1_ENUM_END(AdventureButtonConstant)

H1_ENUM_BEGIN(AdventureScreenConstant)
    LOGICAL_SCREEN_WIDTH = 640,
    LOGICAL_SCREEN_HEIGHT = 480,
    SCROLL_BORDER = 16
H1_ENUM_END(AdventureScreenConstant)

H1_ENUM_BEGIN(AdventureBorderConstant)
    ADVENTURE_VIEWPORT_EXTENT = 480,
    BORDER_EDGE_SIZE = 16,
    BORDER_SIDE_BYTES = 16,
    BORDER_SAVED_SIDE_BYTES = 32,
    BORDER_MIDDLE_END = 464,
    BORDER_BUFFER_SIZE = 0x7400
H1_ENUM_END(AdventureBorderConstant)

H1_ENUM_BEGIN(AdventureLocatorConstant)
    LOCATOR_VISIBLE_COUNT = 4,
    LOCATOR_PAGE_THRESHOLD = 5,
    LOCATOR_PAGE_DENOMINATOR_OFFSET = 4,
    LOCATOR_SCROLL_NO_PAGES_Y = 232
H1_ENUM_END(AdventureLocatorConstant)

H1_ENUM_BEGIN(BottomViewMode)
    BOTTOM_VIEW_NONE = 0,
    BOTTOM_VIEW_NEW_TURN = 1,
    BOTTOM_VIEW_KINGDOM = 2,
    BOTTOM_VIEW_RESOURCE = 5,
    BOTTOM_VIEW_OVERRIDE_DISABLED = 6
H1_ENUM_END(BottomViewMode)

H1_ENUM_BEGIN(BottomViewPanelConstant)
    BOTTOM_VIEW_DRAW_FIRST_WIDGET = 2000,
    BOTTOM_VIEW_DRAW_LAST_WIDGET = 2200,
    BOTTOM_VIEW_PANEL_X = 480,
    BOTTOM_VIEW_PANEL_Y = 392,
    BOTTOM_VIEW_PANEL_WIDTH = 143,
    BOTTOM_VIEW_PANEL_HEIGHT = 71
H1_ENUM_END(BottomViewPanelConstant)

H1_ENUM_BEGIN(AdventureScrollConstant)
    SCROLL_MIN_ORIGIN = -7,
    SCROLL_MAX_ORIGIN = 64,
    SCROLL_TICK_INTERVAL = 70,
    HOVER_SCROLL_FRAME_FIRST = 32,
    HOVER_SCROLL_FRAME_END = 40
H1_ENUM_END(AdventureScrollConstant)

H1_ENUM_BEGIN(AdventurePanelDialogConstant)
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
H1_ENUM_END(AdventurePanelDialogConstant)

H1_ENUM_BEGIN(AdventureSpellType)
    SPELL_VIEW_MINES = 19,
    SPELL_VIEW_RESOURCES = 20,
    SPELL_VIEW_ARTIFACTS = 21,
    SPELL_VIEW_TOWNS = 22,
    SPELL_VIEW_HEROES = 23,
    SPELL_VIEW_ALL = 24,
    SPELL_IDENTIFY_HERO = 25,
    SPELL_SUMMON_BOAT = 26,
    SPELL_DIMENSION_DOOR = 27,
    SPELL_TOWN_GATE = 28,
    SPELL_TRAVEL_MOBILITY_COST = 12
H1_ENUM_END(AdventureSpellType)

H1_ENUM_BEGIN(AdventurePanelButtonConstant)
    ADVMGR_PANEL_BUTTON_FIRST = 1,
    ADVMGR_PANEL_BUTTON_LAST = 6
H1_ENUM_END(AdventurePanelButtonConstant)
// clang-format on

// Buka 2.1's unconditional six-button enable/disable broadcast.
#define SET_ADVENTURE_BUTTON_FLAGS(message, window, cmd)                                           \
    ((message).type = MESSAGE_WIDGET,                                                              \
     (message).payload.widget.command = (cmd),                                                     \
     (message).payload.widget.data.value = WIDGET_FLAG_ENABLED,                                    \
     (message).payload.widget.id = ADVMGR_PANEL_BUTTON_FIRST,                                      \
     (window)->BroadcastMessage(message),                                                          \
     (message).payload.widget.id = ADVMGR_PANEL_BUTTON_FIRST + 1,                                  \
     (window)->BroadcastMessage(message),                                                          \
     (message).payload.widget.id = ADVMGR_PANEL_BUTTON_FIRST + 2,                                  \
     (window)->BroadcastMessage(message),                                                          \
     (message).payload.widget.id = ADVMGR_PANEL_BUTTON_FIRST + 3,                                  \
     (window)->BroadcastMessage(message),                                                          \
     (message).payload.widget.id = ADVMGR_PANEL_BUTTON_FIRST + 4,                                  \
     (window)->BroadcastMessage(message),                                                          \
     (message).payload.widget.id = ADVMGR_PANEL_BUTTON_LAST,                                       \
     (window)->BroadcastMessage(message))

 // donor PoL RVA 0x00056350; preferred Buka symbol ??0advManager@@QAE@XZ
 // donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
 // evidence: graph:2;base=0.538995;margin=0.239070;shape=0.344;size=0.950;calls=1.000;alternate=pol20:void advManager::constructor(void)@0x00056350
 VA(0x004252c0, 0x2cc)
advManager::advManager(void) {
    int i;

    m_groundTiles = 0;
    m_puzzleIcon = 0;
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
    m_adventureBorder = 0;
    for (i = 0; i < ADVMGR_OBJECT_ICON_COUNT; i++)
        m_objectIcons[i] = 0;
    for (i = 0; i < ADVMGR_HERO_ICON_COUNT; i++)
        m_heroIcons[i] = 0;
    for (i = 0; i < ADVMGR_PLAYER_COLOR_COUNT; i++) {
        m_flagIcons[i] = 0;
        m_boatFlagIcons[i] = 0;
    }
    for (i = 0; i < ADVMGR_ENVIRONMENT_SOUND_COUNT; i++)
        m_loopingSamples[i] = 0;
    for (i = 0; i < ADVMGR_CURSOR_SAMPLE_COUNT; i++)
        m_cursorSamples[i] = 0;
    m_puzzleIcon = 0;
    m_cloudOverlayIcon = 0;
    m_boatShadowIcon = 0;
    m_groundTiles = 0;
    m_cloudTiles = 0;
    m_stoneTiles = 0;
    m_adventureWindow = 0;
    m_visibilityMap = 0;
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
short advManager::Open(short) {
    return 0;
}

// donor PoL RVA 0x00057028; preferred Buka symbol ?Close@advManager@@UAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.555933;margin=0.510723;shape=0.387;size=0.995;calls=0.909;alternate=pol20:void advManager::Close(void);   // virtual [override (implements baseManager pure virtual)]@0x00057028
VA(0x00426451, 0x3bd)
void advManager::Close(void) {}

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
    return 0;
}

// donor PoL RVA 0x00057d6c; preferred Buka symbol ?Main@advManager@@UAEHAAUtag_message@@@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.507706;margin=0.523825;shape=0.297;size=0.996;calls=0.852;alternate=pol20:int advManager::Main(struct tag_message &);   // virtual [override (implements baseManager pure virtual)]@0x00057d6c
VA(0x00426eee, 0xe10)
short advManager::Main(struct tag_message&) {
    return 0;
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
int advManager::ProcessSelect(struct tag_message*, class mapCell**) {
    return 0;
}

// donor PoL RVA 0x00059c19; preferred Buka symbol ?ProcessDeSelect@advManager@@QAEHPAUtag_message@@PAHPAPAVmapCell@@@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.346709;margin=0.551065;shape=0.313;size=0.541;calls=0.500;alternate=pol20:int advManager::ProcessDeSelect(struct tag_message *, int *, class mapCell * *)@0x00059c19
VA(0x00428b59, 0x1ea)
int advManager::ProcessDeSelect(struct tag_message*, int*, class mapCell**) {
    return 0;
}

// donor PoL RVA 0x0005a07c; preferred Buka symbol ?ProcessSearch@advManager@@QAEHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.676641;margin=0.529889;shape=0.365;size=0.904;calls=0.935;strings=%s%s|DIGSOUND.82M;alternate=pol20:int advManager::ProcessSearch(int, int)@0x0005a07c
VA(0x00428d43, 0x49b)
int advManager::ProcessSearch(int, int) {
    return 0;
}

// donor PoL RVA 0x0005a644; preferred Buka symbol ?ProcessHover@advManager@@QAEHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.464646;margin=0.430920;shape=0.299;size=0.767;calls=0.971;alternate=pol20:int advManager::ProcessHover(int, int)@0x0005a644
VA(0x004291de, 0xc02)
int advManager::ProcessHover(struct tag_message*) {
    return 0;
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
    gpMouseManager->BeginScreenUpdate(gpWindowManager->m_screen, m_updateMinX, m_updateMinY, cursorUpdate);
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
    gpMouseManager->EndScreenUpdate();
    Process1WindowsMessage();
}

// donor PoL RVA 0x0005b2ae; preferred Buka symbol ?CompleteDraw@advManager@@QAEXHHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:8;base=0.459172;margin=1.327145;shape=0.409;size=0.723;calls=0.706;alternate=pol20:void advManager::CompleteDraw(int, int, int, int)@0x0005b2ae
VA(0x0042a045, 0x359)
void advManager::CompleteDraw(short, short, int) {}

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
void advManager::DrawCell(int, int, int, int, int, int, int) {}

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
void advManager::UpdateRadar(int, int) {}

// donor PoL RVA 0x0005f127; preferred Buka symbol ?QuickInfo@advManager@@QAEXHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.437984;margin=0.160503;shape=0.304;size=0.325;calls=0.543;strings=qwikinfo.bin;alternate=pol20:void advManager::QuickInfo(int, int)@0x0005f127
VA(0x0042bcc8, 0x596)
void advManager::QuickInfo(int, int) {}

// donor PoL RVA 0x00060465; preferred Buka symbol ?UpdateHeroLocator@advManager@@QAEXHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.539169;margin=0.099782;shape=0.447;size=0.791;calls=0.917;alternate=pol20:void advManager::UpdateHeroLocator(int, int, int)@0x00060465
VA(0x0042c25e, 0x3c8)
void advManager::UpdateHeroLocator(int, int, int) {}

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
void advManager::UpdateTownLocators(int, int) {}

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
        if (m_bottomViewPrimaryWidgets[widgetIndex] != 0) {
            m_adventureWindow->RemoveWidget(m_bottomViewPrimaryWidgets[widgetIndex]);
            delete m_bottomViewPrimaryWidgets[widgetIndex];
        }
        if (m_bottomViewSecondaryWidgets[widgetIndex] != 0) {
            m_adventureWindow->RemoveWidget(m_bottomViewSecondaryWidgets[widgetIndex]);
            delete m_bottomViewSecondaryWidgets[widgetIndex];
        }
        m_bottomViewPrimaryWidgets[widgetIndex] = 0;
        m_bottomViewSecondaryWidgets[widgetIndex] = 0;
    }
    iCurBottomViewEnemy = -1;
    iCurBottomView = BOTTOM_VIEW_NONE;
    iLastAnimFrame = -1;
}

// donor PoL RVA 0x00060e95; preferred Buka symbol ?UpdBottomViewEnemyTurn@advManager@@QAEHXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:1;base=0.738388;margin=0.356312;shape=0.508;size=0.910;calls=0.857;strings=brcrest.icn|hourglas.icn|stonback.icn;alternate=pol20:int advManager::UpdBottomViewEnemyTurn(void)@0x00060e95
VA(0x0042cc7e, 0x5bf)
signed char advManager::UpdBottomViewEnemyTurn(void) { return 0; }

// donor PoL RVA 0x000613b0; preferred Buka symbol ?UpdBottomViewNewTurn@advManager@@QAEHXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.861364;margin=0.145966;shape=0.778;size=0.910;calls=0.840;strings=%s: %d|%s: %d  %s: %d|bigfont.fnt;alternate=pol20:int advManager::UpdBottomViewNewTurn(void)@0x000613b0
VA(0x0042d23d, 0x3e0)
signed char advManager::UpdBottomViewNewTurn(void) { return 0; }

// donor PoL RVA 0x00061716; preferred Buka symbol ?UpdBottomViewResMsg@advManager@@QAEHXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.790087;margin=0.239399;shape=0.649;size=0.884;calls=0.793;strings=resource.icn|smalfont.fnt|stonback.icn;alternate=pol20:int advManager::UpdBottomViewResMsg(void)@0x00061716
VA(0x0042d61d, 0x3fa)
signed char advManager::UpdBottomViewResMsg(void) { return 0; }

// donor PoL RVA 0x00061a75; preferred Buka symbol ?UpdBottomViewKingdom@advManager@@QAEHXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.769757;margin=0.069402;shape=0.564;size=0.915;calls=0.850;strings=ressmall.icn|smalfont.fnt|stonback.icn;alternate=pol20:int advManager::UpdBottomViewKingdom(void)@0x00061a75
VA(0x0042da17, 0x3ce)
signed char advManager::UpdBottomViewKingdom(void) { return 0; }

// donor PoL RVA 0x00061dd8; preferred Buka symbol ?UpdBottomViewHero@advManager@@QAEHXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.635193;margin=0.117414;shape=0.302;size=0.961;calls=0.625;strings=mons32.icn|smalfont.fnt|stonback.icn;alternate=pol20:int advManager::UpdBottomViewHero(void)@0x00061dd8
VA(0x0042dde5, 0x62c)
signed char advManager::UpdBottomViewHero(void) { return 0; }

// donor PoL RVA 0x0006235b; preferred Buka symbol ?HeroQuickView@advManager@@QAEXHHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:13;base=0.654225;margin=1.910013;shape=0.309;size=0.949;calls=0.880;strings=mons32.icn|qhero0.bin|qhero1.bin;alternate=pol20:void advManager::HeroQuickView(int, int, int, int)@0x0006235b
VA(0x0042e411, 0xd46)
void advManager::HeroQuickView(int, int, int, int) {}

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
void advManager::TownQuickView(int, int, int, int) {}

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
    cell->m_triggerType = 0xbd;
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
        0,
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
void advManager::DoHeroKnob(void) {}

// donor PoL RVA 0x000648d9; preferred Buka symbol ?DoTownKnob@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.410865;margin=0.000000;shape=0.246;size=0.742;calls=0.733;alternate=pol20:void advManager::DoTownKnob(void)@0x000648d9
VA(0x00430a75, 0x290)
void advManager::DoTownKnob(void) {}

// donor PoL RVA 0x0006a1dd; preferred Buka symbol ?ViewPuzzle@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.634399;margin=0.712778;shape=0.353;size=0.798;calls=0.917;strings=advmice.mse|puzzle.icn|viewpuzl.bin;alternate=pol20:void advManager::ViewPuzzle(void)@0x0006a1dd
VA(0x00430d05, 0x3da)
void advManager::ViewPuzzle(void) {}

// HoMM1 PuzzleDraw redraws the 15x15 cells itself, overlaying the puzzle's
// visible object/overlay frames and marking the target cell.
VA(0x004310df, 0x236)
void advManager::PuzzleDraw(int left, int top, int markX, int markY) {
    unsigned char tileset;
    mapCell* cell;
    int x;
    int y;
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
        caster = 0;

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
            NormalDialog("Enemy Heroes are now fully identifiable.", 1, 0x61, 0x91, -1, 0, -1, 0, -1);
            break;
        case SPELL_SUMMON_BOAT:
            SummonBoat();
            break;
        case SPELL_DIMENSION_DOOR:
        case SPELL_TOWN_GATE:
            if (caster->m_remainingMobility == 0) {
                NormalDialog("Your hero is too tired to cast this spell today.  Try again tomorrow.",
                             1, -1, -1, -1, 0, -1, 0, -1);
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
void advManager::ViewWorld(signed char, signed char, signed char) {}

// HoMM1-only helper: refresh the saved screen copy with the pointer hidden.
VA(0x0043262e, 0x41)
void advManager::GrabScreen(void) {
    gpMouseManager->ReallyHidePointer();
    GrabScreenBitmap(gpWindowManager->m_screen, 0, 0);
    gpMouseManager->ReallyShowPointer();
}

// clang-format off
H1_ENUM_BEGIN(ControlPanelDialogConstant)
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
H1_ENUM_END(ControlPanelDialogConstant)
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
    if (cPanel == 0)
        MemError();
    SetWinText(cPanel, 3);
    if (gbRemoteOn) {
        message.type = MESSAGE_WIDGET;
        message.payload.widget.id = CONTROL_NEW_GAME;
        message.payload.widget.command = WIDGET_COMMAND_SET_FLAGS;
        message.payload.widget.data.value = WIDGET_COMMAND_DIMMED;
        cPanel->BroadcastMessage(message);
        message.payload.widget.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.payload.widget.data.value = WIDGET_FLAG_ENABLED;
        cPanel->BroadcastMessage(message);
        message.payload.widget.id = CONTROL_LOAD_GAME;
        message.payload.widget.command = WIDGET_COMMAND_SET_FLAGS;
        message.payload.widget.data.value = WIDGET_COMMAND_DIMMED;
        cPanel->BroadcastMessage(message);
        message.payload.widget.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.payload.widget.data.value = WIDGET_FLAG_ENABLED;
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
    message.payload.widget.command = WIDGET_COMMAND_SET_FRAME;
    message.payload.widget.id = CONTROL_MUSIC_VOLUME;
    message.payload.widget.data.value = gConfig.musicVolume ? 11 : 10;
    cPanel->BroadcastMessage(message);
    message.payload.widget.id = CONTROL_SOUND_VOLUME;
    message.payload.widget.data.value = gConfig.soundVolume ? 13 : 12;
    cPanel->BroadcastMessage(message);
    message.payload.widget.id = CONTROL_WALK_SPEED;
    message.payload.widget.data.value = gConfig.walkSpeed + 14;
    cPanel->BroadcastMessage(message);
    message.payload.widget.id = CONTROL_MUSIC_SOURCE;
    message.payload.widget.data.value = gConfig.musicSource + 27;
    cPanel->BroadcastMessage(message);
    message.payload.widget.id = CONTROL_SHOW_ROUTE;
    message.payload.widget.data.value = gConfig.showRoute + 21;
    cPanel->BroadcastMessage(message);
    message.payload.widget.id = CONTROL_SHOW_ENEMY_MOVES;
    message.payload.widget.data.value = gbRemoteOn ? 23 : 1 - gConfig.blackoutComputer + 23;
    cPanel->BroadcastMessage(message);
    message.payload.widget.command = WIDGET_COMMAND_SET_TEXT;
    message.payload.widget.id = 8;
    message.payload.widget.data.text = onOffText[gConfig.musicVolume];
    cPanel->BroadcastMessage(message);
    message.payload.widget.id = 9;
    message.payload.widget.data.text = onOffText[gConfig.soundVolume];
    cPanel->BroadcastMessage(message);
    message.payload.widget.id = 10;
    message.payload.widget.data.text = walkSpeedText[gConfig.walkSpeed];
    cPanel->BroadcastMessage(message);
    message.payload.widget.id = 14;
    message.payload.widget.data.text = musicQualityText[gConfig.musicSource];
    cPanel->BroadcastMessage(message);
    message.payload.widget.id = 15;
    message.payload.widget.data.text = onOffText[gConfig.showRoute];
    cPanel->BroadcastMessage(message);
    message.payload.widget.id = 16;
    message.payload.widget.data.text = onOffText[1 - gConfig.blackoutComputer];
    cPanel->BroadcastMessage(message);
    if (!initialDraw)
        cPanel->MoveWindow(0, 0);
}

VA(0x00432bb7, 0x232)
int SaveGame(void) {
    return 0;
}

extern char *gCPanelHelp[];

// Buka 2.1 CPanelHandler plus SystemOptionsHandler's option cycling.
VA(0x00432de9, 0x54b)
short CPanelHandler(struct tag_message &message) {
    signed char changed = 0;
    char question[120];
    signed char handled = 0;
    if (message.type == MESSAGE_WIDGET) {
        if (message.payload.widget.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON) {
            if (IS_WIDGET_SELECTION_NOTIFICATION(message.payload.widget.command)) {
                int helpIndex = -1;
                switch (message.payload.widget.id) {
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
                    NormalDialog(gCPanelHelp[helpIndex], 4, 0xb1, -1, -1, 0, -1, 0, -1);
            }
        } else {
            switch (message.payload.widget.command) {
                case WIDGET_NOTIFY_DESELECT:
                    switch (message.payload.widget.id) {
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
                                NormalDialog(question, 2, 0xb1, 0x50, -1, 0, -1, 0, -1);
                                if (gpWindowManager->m_dialogResult == 0x7806)
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
                    switch (message.payload.widget.id) {
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
                                        1, -1, -1, -1, 0, -1, 0, -1
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
        gpWindowManager->m_dialogResult = message.payload.widget.id;
        message.payload.widget.command = message.payload.widget.id = WIDGET_COMMAND_DIALOG_SELECT;
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
    if (adventurePanel == 0)
        MemError();
    if (gpCurPlayer->CurrentHero() == -1) {
        message.type = MESSAGE_WIDGET;
        message.payload.widget.id = PANEL_SEARCH;
        message.payload.widget.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.payload.widget.data.value = WIDGET_FLAG_ENABLED;
        adventurePanel->BroadcastMessage(message);
        message.payload.widget.id = PANEL_CAST_SPELL;
        adventurePanel->BroadcastMessage(message);
        message.payload.widget.id = PANEL_SEARCH;
        message.payload.widget.command = WIDGET_COMMAND_SET_FLAGS;
        message.payload.widget.data.value = WIDGET_COMMAND_DIMMED;
        adventurePanel->BroadcastMessage(message);
        message.payload.widget.id = PANEL_CAST_SPELL;
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
        if (message.payload.widget.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON) {
            if (IS_WIDGET_SELECTION_NOTIFICATION(message.payload.widget.command)) {
                int helpIndex = PANEL_NO_HELP;
                switch (message.payload.widget.id) {
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
                    NormalDialog(gAPanelHelp[helpIndex], 4, 0xb1, -1, -1, 0, -1, 0, -1);
            }
        } else {
            switch (message.payload.widget.command) {
                case WIDGET_NOTIFY_DESELECT:
                    switch (message.payload.widget.id) {
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
        gpWindowManager->m_dialogResult = message.payload.widget.id;
        message.payload.widget.command = message.payload.widget.id = WIDGET_COMMAND_DIALOG_SELECT;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

VA(0x004337c5, 0x34b)
int DimensionDoorHandler(struct tag_message&) {
    return 0;
}

// donor PoL RVA 0x000654ad; preferred Buka symbol ?ComboDraw@advManager@@QAEHHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.384237;margin=0.212683;shape=0.299;size=0.586;calls=0.778;alternate=pol20:int advManager::ComboDraw(int, int, int)@0x000654ad
VA(0x00433b10, 0xaf6)
signed char advManager::ComboDraw(short, short, int) {
    return 0;
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
void advManager::SetEnvironmentOrigin(int, int, int) {}

// donor PoL RVA 0x000669c6; preferred Buka symbol ?CheckLoadSample@advManager@@QAEXH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:1;base=0.741786;margin=0.490066;shape=0.533;size=0.857;calls=1.000;strings=loop%04d.82M;alternate=pol20:void advManager::CheckLoadSample(int)@0x000669c6
VA(0x0043491d, 0x69)
void advManager::CheckLoadSample(int index) {
    if (m_loopingSamples[index] == 0) {
        TrimLoopingSounds(ADVMGR_ACTIVE_SOUND_COUNT);
        sprintf(gText, "loop%04d.82M", index);
        m_loopingSamples[index] = gpResourceManager->GetSample(gText);
    }
}

// donor PoL RVA 0x00066ef0; preferred Buka symbol ?InsertSound@advManager@@QAEXHHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.476286;margin=0.526376;shape=0.266;size=0.902;calls=0.750;alternate=pol20:void advManager::InsertSound(int, int, int, int)@0x00066ef0
VA(0x00434986, 0x251)
void advManager::InsertSound(int, int, int, int) {}

// donor PoL RVA 0x0006712a; preferred Buka symbol ?TeleportTo@advManager@@QAEXPAVhero@@HHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.494469;margin=0.364782;shape=0.352;size=0.864;calls=0.864;alternate=pol20:void advManager::TeleportTo(class hero *, int, int, int, int)@0x0006712a
VA(0x00434bd7, 0x340)
void advManager::TeleportTo(int, int, int) {}

// donor PoL RVA 0x00067539; preferred Buka symbol ?DimensionDoor@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.671113;margin=0.501597;shape=0.372;size=0.883;calls=0.867;strings=dimdoor.bin;alternate=pol20:void advManager::DimensionDoor(void)@0x00067539
VA(0x00434f17, 0x246)
void advManager::DimensionDoor(void) {}

// donor PoL RVA 0x0006785d; preferred Buka symbol ?TownGate@advManager@@QAEXH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.385467;margin=0.208066;shape=0.290;size=0.657;calls=0.526;alternate=pol20:void advManager::TownGate(int)@0x0006785d
VA(0x0043515d, 0x2a6)
void advManager::TownGate(void) {}

// donor PoL RVA 0x00067c9b; preferred Buka symbol ?SummonBoat@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.502597;margin=0.051626;shape=0.294;size=0.965;calls=0.867;alternate=pol20:void advManager::SummonBoat(void)@0x00067c9b
VA(0x00435403, 0x51c)
void advManager::SummonBoat(void) {}

// donor PoL RVA 0x00068247; preferred Buka symbol ?ShowRoute@advManager@@QAEXHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.438878;margin=0.464254;shape=0.279;size=0.721;calls=1.000;alternate=pol20:void advManager::ShowRoute(int, int, int)@0x00068247
VA(0x0043591f, 0x31b)
void advManager::ShowRoute(int, int, int) {}

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
    msg.payload.widget.id = 10;
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
void advManager::SetInitialMapOrigin(void) {}

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
    return 0;
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
        switch (message.payload.keyboard.keyCode) {
            case 0x3b:
                PopNetBox(0);
                break;

            case 0x10:
                if (message.payload.keyboard.modifiers & 0xc) {
                    message.type = MESSAGE_EXECUTIVE;
                    message.payload.executive.command = EXECUTIVE_COMMAND_TERMINATE_LOOP;
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
    if (m_adventureBorder != 0)
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

    if (m_adventureBorder == 0)
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

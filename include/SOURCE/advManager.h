#ifndef HOMM1_SOURCE_ADVMANAGER_H
#define HOMM1_SOURCE_ADVMANAGER_H

#include <BASE/audioTypes.h>
#include <BASE/baseManager.h>
#include <BASE/message.h>
#include <SOURCE/armySizeNames.h>
#include <SOURCE/artifactTypes.h>
#include <SOURCE/creatureTypes.h>
#include <SOURCE/cursorTypes.h>
#include <SOURCE/dialogTypes.h>
#include <SOURCE/EVENTS.h>
#include <SOURCE/mapCell.h>
#include <SOURCE/mapObjectTypes.h>
#include <SOURCE/playerData.h>
#include <SOURCE/resourceTypes.h>
#include <SOURCE/spellTypes.h>
#include <SOURCE/terrainTypes.h>

class armyGroup;
class backdropWidget;
class hero;
class heroWindow;
class icon;
class iconWidget;
class mapCell;
class sample;
class tileset;
class town;
class widget;
struct RemoteMessage;
struct SMapChange;
struct tag_message;

enum AdventureManagerStorageConstant {
    ADVMGR_BOTTOM_VIEW_WIDGET_COUNT = 12,
    ADVMGR_BOTTOM_VIEW_BACKGROUND = 0,
    ADVMGR_BOTTOM_VIEW_FOREGROUND = 1,
    ADVMGR_BOTTOM_VIEW_ICON_FIRST = 2,
    ADVMGR_BOTTOM_VIEW_HERO_TEXT_FIRST = 1,
    ADVMGR_BOTTOM_VIEW_TEXT = 0,
    ADVMGR_BOTTOM_VIEW_COUNT_TEXT = 1,
    ADVMGR_OBJECT_ICON_COUNT = 21,
    ADVMGR_PANEL_ICON_COUNT = 5,
    ADVMGR_ANIMATION_PHASE_COUNT = 4,
    ADVMGR_HERO_ICON_COUNT = 5,
    ADVMGR_PLAYER_COLOR_COUNT = 4,
    ADVMGR_ACTIVE_SOUND_COUNT = 8,
    ADVMGR_ENVIRONMENT_SOUND_COUNT = 22,
    ADVMGR_CURSOR_SAMPLE_COUNT = 7
};

enum AdventureFrameTimerConstant {
    TIMER_DELAY = 120
};

enum AdventureCombatMonsterCellConstant {
    COMBAT_MONSTER_CELL_NONE = -99,
    COMBAT_MONSTER_CELL_CLEARED = -1,
    COMBAT_MONSTER_CELL_AT_EVENT = -1
};

enum AdventureCursorConstant {
    CURSOR_INVALID_POSITION = -1
};

enum AdventureGiveArtifactConstant {
    GIVE_ARTIFACT_NO_SLOT = -1
};

enum AdventureHeroFrameConstant {
    HERO_FRAME_MIRROR_FLAG = 0x80,
    HERO_FRAME_INDEX_MASK = 0x7f
};

enum AdventureHeroIcon {
    ADVMGR_HERO_ICON_KNIGHT = 0,
    ADVMGR_HERO_ICON_BARBARIAN = 1,
    ADVMGR_HERO_ICON_SORCERESS = 2,
    ADVMGR_HERO_ICON_WARLOCK = 3,
    ADVMGR_HERO_ICON_CLASS_END = 4,
    ADVMGR_HERO_ICON_BOAT = 4
};

enum AdventureCommand {
    ADVMGR_COMMAND_NONE = -1,
    ADVMGR_COMMAND_MOVE_TO = 1,
    ADVMGR_COMMAND_HERO_VIEW = 2,
    ADVMGR_COMMAND_TOWN_VIEW = 3,
    ADVMGR_COMMAND_SELECT_HERO = 4,
    ADVMGR_COMMAND_SELECT_TOWN = 5,
    ADVMGR_COMMAND_OCCUPIED_TOWN_VIEW = 6,
    ADVMGR_COMMAND_CONTINUE_ROUTE = 7
};

enum AdventureSpellConstant {
    SPELL_TRAVEL_MOBILITY_COST = 12
};

enum AdventureSearchConstant {
    ADVMGR_SEARCH_VIEW_CENTER = -1,
    DIG_HOLE_FRAME = 1
};

enum AdventureLocatorConstant {
    LOCATOR_SLOT_CURRENT_HERO = -1,
    LOCATOR_VISIBLE_COUNT = 4,
    LOCATOR_PAGE_THRESHOLD = 5,
    LOCATOR_PAGE_DENOMINATOR_OFFSET = 4,
    LOCATOR_SCROLL_NO_PAGES_Y = 232,
    LOCATOR_SCROLL_BASE_Y = 195,
    LOCATOR_HERO_SCROLL_SPAN = 73,
    LOCATOR_TOWN_SCROLL_SPAN = 74,
    LOCATOR_SCROLL_MOUSE_BASE_Y = LOCATOR_SCROLL_BASE_Y - 1,
    LOCATOR_SCROLL_MOUSE_SPAN = 92
};

enum AdventureControl {
ADVENTURE_CONTROL_NEXT_HERO = 1,
    ADVENTURE_CONTROL_CONTINUE_ROUTE = 2, ADVENTURE_CONTROL_OVERVIEW = 3,
    ADVENTURE_CONTROL_END_TURN = 4, ADVENTURE_CONTROL_ADVENTURE_OPTIONS = 5,
    ADVENTURE_CONTROL_GAME_OPTIONS = 6, ADVENTURE_CONTROL_RADAR = 9,
    ADVENTURE_CONTROL_MAP_VIEW = 10, ADVENTURE_CONTROL_TOWN_LOCATOR_1 = 16,
    ADVENTURE_CONTROL_TOWN_LOCATOR_2 = 17, ADVENTURE_CONTROL_TOWN_LOCATOR_3 = 18,
    ADVENTURE_CONTROL_TOWN_LOCATOR_4 = 19, ADVENTURE_CONTROL_HERO_PAGE_PREVIOUS = 20,
    ADVENTURE_CONTROL_HERO_PAGE_NEXT = 21, ADVENTURE_CONTROL_HERO_SCROLL = 22,
    ADVENTURE_CONTROL_TOWN_PAGE_PREVIOUS = 23, ADVENTURE_CONTROL_TOWN_PAGE_NEXT = 24,
    ADVENTURE_CONTROL_TOWN_SCROLL = 25, ADVENTURE_CONTROL_HERO_KNOB = 26,
    ADVENTURE_CONTROL_TOWN_KNOB = 27, ADVENTURE_CONTROL_HERO_LOCATOR_1 = 105,
    ADVENTURE_CONTROL_HERO_LOCATOR_2 = 112, ADVENTURE_CONTROL_HERO_LOCATOR_3 = 119,
    ADVENTURE_CONTROL_HERO_LOCATOR_4 = 126 };

    enum AdventurePanelButtonConstant {
    ADVMGR_PANEL_BUTTON_FIRST = 1,
    ADVMGR_PANEL_BUTTON_LAST = 6
};

#define SET_ADVENTURE_BUTTON_FLAGS(message, window, cmd)                                           \
    ((message).type = MESSAGE_WIDGET,                                                              \
     (message).command = (cmd),                                                                    \
     (message).value = WIDGET_FLAG_ENABLED,                                                        \
     (message).id = ADVMGR_PANEL_BUTTON_FIRST,                                                     \
     (window)->BroadcastMessage(message),                                                          \
     (message).id = ADVMGR_PANEL_BUTTON_FIRST + 1,                                                 \
     (window)->BroadcastMessage(message),                                                          \
     (message).id = ADVMGR_PANEL_BUTTON_FIRST + 2,                                                 \
     (window)->BroadcastMessage(message),                                                          \
     (message).id = ADVMGR_PANEL_BUTTON_FIRST + 3,                                                 \
     (window)->BroadcastMessage(message),                                                          \
     (message).id = ADVMGR_PANEL_BUTTON_FIRST + 4,                                                 \
     (window)->BroadcastMessage(message),                                                          \
     (message).id = ADVMGR_PANEL_BUTTON_LAST,                                                      \
     (window)->BroadcastMessage(message))

struct adventureSoundCell {
    i32 soundId;
    i32 distance;
};

enum AdventureDrawMask {
    ADVMGR_DRAW_GROUND = 0x01,
    ADVMGR_DRAW_OBJECT = 0x02,
    ADVMGR_DRAW_OVERLAY = 0x04,
    ADVMGR_DRAW_HERO = 0x08,
    ADVMGR_DRAW_CLOUD = 0x20
};

class advManager : public baseManager {
public:
    i8 m_pendingCommand;
    class widget* m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_WIDGET_COUNT];
    class widget* m_bottomViewSecondaryWidgets[ADVMGR_BOTTOM_VIEW_WIDGET_COUNT];
    class heroWindow* m_adventureWindow;
    i8* m_routeMap;
    b8 m_routeShown;
    i8 m_currentTerrain;
    char m_unused9b[4];
    class mapCell (*m_mapData)[MAP_CELL_GRID_SIZE];
    class iconWidget* m_scrollLeftButton;
    class iconWidget* m_scrollRightButton;
    class backdropWidget* m_panelBackdrops[ADVMGR_PANEL_ICON_COUNT];
    u8* m_adventureBorder;
    char m_unusedc3[4];
    class tileset* m_groundTiles;
    class tileset* m_cloudTiles;
    class tileset* m_stoneTiles;
    class icon* m_objectIcons[ADVMGR_OBJECT_ICON_COUNT];
    class icon* m_radarIcon;
    class icon* m_cloudOverlayIcon;
    i16 m_mapOriginX;
    i16 m_mapOriginY;
    i16 m_previousOriginX;
    i16 m_previousOriginY;
    i16 m_hoverCellX;
    i16 m_hoverCellY;
    i16 m_commandTargetX;
    i16 m_commandTargetY;
    i16 m_scrollOffsetX;
    i16 m_scrollOffsetY;
    i16 m_animationFrame;
    i16 m_flagFrameCounter;
    i8 m_animationPhases[ADVMGR_ANIMATION_PHASE_COUNT];
    class icon* m_heroIcons[ADVMGR_HERO_ICON_COUNT];
    class icon* m_shadowIcon;
    class icon* m_flagIcons[ADVMGR_PLAYER_COLOR_COUNT];
    class icon* m_boatFlagIcons[ADVMGR_PLAYER_COLOR_COUNT];
    b8 m_cursorActive;
    b8 m_drawHeroShadows;
    u8 m_cursorType;
    i8 m_cursorDirection;
    i16 m_cursorFrame;
    i16 m_cursorFrameCount;
    i16 m_cursorCycle;
    i16 m_cursorTurning;
    i16 m_cursorMapX;
    i16 m_previousCursorMapX;
    i16 m_cursorMapY;
    i16 m_previousCursorMapY;
    b8 m_comboHeroDrawn;
    b32 m_heroContextLocked;
    b32 m_townContextLocked;
    b8 m_forceCompleteDraw;
    i8 m_combatMonsterX;
    i8 m_combatMonsterY;
    b8 m_combatMonsterFacingLeft;
    i32 m_activeSoundMask;
    adventureSoundCell m_activeSounds[ADVMGR_ACTIVE_SOUND_COUNT];
    class sample* m_loopingSamples[ADVMGR_ENVIRONMENT_SOUND_COUNT];
    class sample* m_cursorSamples[ADVMGR_CURSOR_SAMPLE_COUNT];
    b8 m_identifyHeroActive;
    b8 m_heroesLogoShown;
    i16 m_messageTypeMask;
    advManager(void);
    ~advManager();
    virtual i16 Open(i16 id) ;
    virtual void Close(void) ;
    virtual i16 Main(struct tag_message& message) ;
    void StartCursor(i8 direction);
    void StopCursor(b8 stopSound);
    void DrawCursor(void);
    i16 GetCursorBaseFrame(i16 direction);
    void TurnTo(i8 direction);
    b32 GetMoveShowIt(i8 direction);
    class mapCell* MoveHero(
        i8 direction,
        b8 stopAfterMove,
        i32* eventX,
        i32* eventY,
        b32* outOfMobility,
        b8 processEvent,
        b8* adjacentMonster
    );
    void CheckAdjacentMon(b8* adjacentMonster);
    i16 ValidMoveWithEvent(class hero* movingHero, i16 direction);
    i16 ValidMove(i16 direction);
    void MoveOrigin(i16 directionX, i16 directionY);
    void ViewWorld(i8 spellType, b8 drawAllObjects, b8 drawAllTerrains);
    void GetCursorSampleSet(i32 sampleSet);
    class mapCell* DoAdvCommand(void);
    i32 GetCommandTargetX(void) {
        return m_commandTargetX;
    }
    i32 GetCommandTargetY(void) {
        return m_commandTargetY;
    }
    void Reseed(i32, i32);
    i32 ProcessSelect(struct tag_message* message, class mapCell** eventCell);
    i32 ProcessDeSelect(struct tag_message* message, i32* result, class mapCell** eventCell);
    b32 ProcessSearch(i32 x, i32 y);
    i32 ProcessHover(struct tag_message* message);
    void UpdateScreen(b8 cursorUpdate, b8 forceUpdate);
    void CompleteDraw(i16 originX, i16 originY, b32 forceDraw);
    void CompleteDraw(b32 forceDraw);
    i32 GetCloudLookup(i32 x, i32 y);
    void DrawCell(
        i16 mapX,
        i16 mapY,
        i16 screenX,
        i16 screenY,
        i8 drawMask,
        b8 drawingPuzzle,
        b8 forceDraw
    );
    class mapCell* GetCell(i16 x, i16 y);
    void UpdateRadar(b8 updateScreen, b32 partial);
    void QuickInfo(i16 cellX, i16 cellY);
    void UpdateHeroLocator(i32 locatorSlot, b8 drawWindow, b8 updateScreen);
    void UpdateHeroLocators(b8 drawWindow, i8 updateScreen);
    void UpdateTownLocators(b8 drawWindow, i8 updateScreen);
    void UpdBottomView(b8 forceUpdate, b8 drawWindow, b8 updateScreen);
    void ClearBottomView(void);
    b8 UpdBottomViewEnemyTurn(void);
    b8 UpdBottomViewNewTurn(void);
    b8 UpdBottomViewResMsg(void);
    b8 UpdBottomViewKingdom(void);
    b8 UpdBottomViewHero(void);
    void HeroQuickView(i8 heroId, i8 locatorSlot, i16 windowX, i16 windowY);
    char* GetArmySizeName(i16 armySize, i8 grammar);
    void TownQuickView(i8 townId, i8 locatorSlot, i16 windowX, i16 windowY);
    void RedrawAdvScreen(b32 update);
    void GiveTakeArtifactStat(
        class hero* targetHero,
        i8 artifact,
        i8 take
    );
    void DeactivateCurrTown(void);
    void DeactivateCurrHero(void);
    void MobilizeCurrHero(b32 update);
    void DemobilizeCurrHero(void);
    void SetTownContext(i8 townId);
    void SetHeroContext(i8 heroId, b8 update);
    void DoHeroKnob(void);
    void DoTownKnob(void);
    void CastSpell(i8 spell);
    void GrabScreen(void);
    void CheckCastSpell(void);
    b8 ComboDraw(i16 originX, i16 originY, b8 animate);
    b8 ComboDraw(b32 animate);
    void SetEnvironmentOrigin(i16 originX, i16 originY, i16 stopSounds);
    void CheckLoadSample(i32 index);
    void InsertSound(i16 x, i16 y, i16 distance, i8 soundLayer);
    void TeleportTo(i32 x, i32 y, i32);
    void DimensionDoor(void);
    void TownGate(void);
    void SummonBoat(void);
    void ShowRoute(b32 redraw, i32, b32 updateButton);
    void HideRoute(b32 redraw, b32 clearDestination, b32 updateButton);
    void CheckDimHero(void);
    void CheckDimNextHeroBut(void);
    void SeedTo(i32 targetX, i32 targetY);
    void ForceNewHover(void);
    void ScreenScroll(i8 direction, b32 updatePointer);
    void CheckScreenScroll(void);
    b32 MouseInScrollZone(void);
    void SetInitialMapOrigin(void);
    void LoadRemote(void);
    RemoteMessage* CheckHandleNet(void);
    i16 CheckHandleNetPlayerWait(struct tag_message& message, b8 doMain);
    void TrimLoopingSounds(i32 maxSamples);
    void DisableButtons(void);
    void EnableButtons(void);
    void SaveAdventureBorder(void);
    void DrawAdventureBorder(void);
    i8 FindAdjacentMonster(
        i32 originX,
        i32 originY,
        i32* monsterX,
        i32* monsterY,
        i32 excludedX,
        i32 excludedY
    );
    void ViewPuzzle(void);
    void PuzzleDraw(i32 left, i32 top, i32 markX, i32 markY);
    void AdvPanel(void);
    i16 ControlPanel(void);
    void DoEvent(class mapCell* cell, i32 x, i32 y);
    void EraseObj(class mapCell* cell, i32 x, i32 y);
    void HeroSwap(class hero* firstHero, class hero* secondHero);
    void TownEvent(class mapCell* cell, i32 x, i32 y);
    void EventSound(i16 eventType, i16 eventData);
    void EventWindow(
        i16 eventId,
        i32 buttons,
        char* text,
        i32 type1,
        i32 value1,
        i32 type2,
        i32 value2,
        i32 showOrText
    );
    i32 GiveRandomArtifact(class mapCell* cell, class hero* eventHero);
    i32 GiveExperience(class hero* eventHero, i32 experience, b8 checkLevel);
    void GiveResource(class hero* eventHero, i8 resource, i16 amount);
    i16 GiveArtifact(class hero* eventHero, i8 artifact);
    void RecruitEvent(
        class hero* eventHero,
        i32 creatureType,
        class mapCell* cell
    );
    b8 GhostEvent(
        class hero* eventHero,
        class mapCell* cell,
        i32 textId,
        i32 x,
        i32 y
    );
    void HouseEvent(class hero* eventHero, class mapCell* cell);
    i8 CombatMonsterEvent(
        class hero* eventHero,
        i8 monsterType,
        i16 monsterCount,
        class mapCell* cell,
        i32 x,
        i32 y,
        b8 heroDefends,
        i32 combatX,
        i32 combatY
    );
    void TransferArtifacts(class hero* sourceHero, class hero* destHero);
    void HeroLoses(class hero* lostHero);
    void DoWhirlpool(class hero* eventHero);
    void FizzleCenter(i32 fizzleType);
    void DoAIEvent(class mapCell* cell, class hero* eventHero, i32 x, i32 y);
    void PlayerMonsterInteract(
        class mapCell* cell,
        class mapCell* combatCell,
        class hero* eventHero,
        b8* removeMonsterObject,
        i32 x,
        i32 y,
        b8 heroDefends,
        i32 combatX,
        i32 combatY
    );
    void
    ComputerMonsterInteract(class mapCell* cell, class hero* eventHero, b8* removeMonsterObject);
    i32 DoNetCombat(RemoteMessage* packet);
    i32 DoCombat(
        i32 x,
        i32 y,
        class hero* firstHero,
        class armyGroup* firstArmy,
        class town* combatTown,
        class hero* secondHero,
        class armyGroup* secondArmy,
        i32 setupCombatX,
        i32 setupCombatY,
        i32 randomSeed,
        b8 processLosses
    );
    void SendHeroTownData(
        i32 x,
        i32 y,
        class hero* firstHero,
        class armyGroup* firstArmy,
        class town* combatTown,
        class hero* secondHero,
        class armyGroup* secondArmy,
        i32 setupCombatX,
        i32 setupCombatY,
        i32 randomSeed,
        i8 remotePlayer,
        i8 combatResult,
        b8 retreatWin,
        b8 combatSurrender
    );
    void ReceiveHeroTownData(
        RemoteMessage* packet,
        i32* remotePlayer,
        i32* x,
        i32* y,
        class hero** firstHero,
        class armyGroup** firstArmy,
        class town** combatTown,
        class hero** secondHero,
        class armyGroup** secondArmy,
        i32* setupCombatX,
        i32* setupCombatY,
        i32* randomSeed,
        i8* combatResult,
        b8* retreatWin,
        b8* combatSurrender
    );
};

i16 APanelHandler(struct tag_message& message);
void UpdateCPanel(b8 initialDraw);
i8 SaveGame(void);
i16 CPanelHandler(struct tag_message& message);

enum ForcedMusicConstant {
    FORCED_MUSIC_IDLE = -1
};
extern i32 gLastScrollTime;
extern b32 gForceUpdate;
#define TERRAIN_MUSIC_TRACK(terrain) (terrain)

enum BottomViewMode {
    BOTTOM_VIEW_NONE = 0,
    BOTTOM_VIEW_NEW_TURN = 1,
    BOTTOM_VIEW_KINGDOM = 2,
    BOTTOM_VIEW_HERO = 3,
    BOTTOM_VIEW_ENEMY_TURN = 4,
    BOTTOM_VIEW_RESOURCE = 5,
    BOTTOM_VIEW_OVERRIDE_DISABLED = 6
};
extern i32 gCurBottomView;
extern i32 gBottomViewOverride;
extern i32 gCurBottomViewEnemy;
extern i32 gLastAnimFrame;
extern i32 gSandAnim;
extern i32 gLastHourGlassPhase;
extern i32 gLastHourGlassUpdateTime;
extern const i32 gEnvironmentVolume[];
enum AdventureUpdateLimitConstant {
    UPDATE_NONE = -1,
    CURSOR_CELL_NONE = -1
};

extern i32 gLimitUpdMinX;
extern i32 gLimitUpdMinY;
extern i32 gLimitUpdMaxX;
extern i32 gLimitUpdMaxY;
extern class heroWindow* gAdventurePanel;
extern b8 gPrefsChanged;
extern b8 gFreshSave;
extern i8 gComboDraw[][17];
extern i32 gTriggerX;
extern i32 gTriggerY;
extern b8 gMoveSoundMade;

extern i8 gEveryOther;
extern i8 gSavedCursorDirection;
extern i16 gSavedCursorBaseFrame;
extern i16 gSavedCursorFrameCount;
extern i16 gSavedCursorCycle;
extern i16 gSavedCursorTurning;
extern i16 gStepDelay[WALK_SPEED_COUNT];
extern i16 gPixelsPerStep[WALK_SPEED_COUNT];
extern i16 gStepScrollStart[];
extern i32 gFrameStep;

struct SMapChange {
    char _pad[64];
};
extern char gArmySizeName[];
extern i32 gCurHourGlassPhase;

enum AdventureButtonConstant {
    BUTTON_BROADCAST_ARG = 1,
    PANEL_CONTINUE_ROUTE = 2
};

enum AdventureScreenConstant {
    SCROLL_BORDER = 16
};

enum AdventureBorderConstant {
    ADVENTURE_VIEWPORT_EXTENT = 480,
    BORDER_EDGE_SIZE = 16,
    BORDER_SIDE_BYTES = 16,
    BORDER_SAVED_SIDE_BYTES = 32,
    BORDER_MIDDLE_END = ADVENTURE_VIEWPORT_EXTENT - BORDER_EDGE_SIZE,
    ADVENTURE_VIEWPORT_INNER_SIZE = BORDER_MIDDLE_END - BORDER_EDGE_SIZE,
    BORDER_BUFFER_SIZE = 0x7400
};

enum AdventureLocatorWidget {
    HERO_LOCATOR_WIDGET_BASE = 100,
    HERO_LOCATOR_WIDGET_STRIDE = 7,
    HERO_LOCATOR_MOBILITY = 1,
    HERO_LOCATOR_PORTRAIT = 2,
    HERO_LOCATOR_BUTTON = 5,
    HERO_LOCATOR_HIGHLIGHT = 6,
    TOWN_LOCATOR_HIGHLIGHT_FIRST = 32,
    LOCATOR_HIGHLIGHT_COLOR = 0xc5
};

enum AdventureRouteCell {
    ROUTE_CELL_NONE = 0,
    ROUTE_CELL_FRAME_MASK = 0x1f,
    ROUTE_CELL_FLIPPED = 0x20,
    ROUTE_CELL_DESTINATION = 14,
    ROUTE_CELL_REACHABLE_OFFSET = 14
};

#define ADVMGR_ROUTE_AT(column, row) (*(m_routeMap + (column) + (row) * MAP_CELL_GRID_SIZE))

enum AdventureLocatorFrame {
    LOCATOR_FRAME_EMPTY_TOWN_FIRST = 4,
    LOCATOR_FRAME_HERO = 8,
    LOCATOR_FRAME_TOWN_FIRST = 12,
    LOCATOR_FRAME_CASTLE_OFFSET = 4
};

enum BottomViewPanelConstant {
    BOTTOM_VIEW_DRAW_FIRST_WIDGET = 2000,
    BOTTOM_VIEW_DRAW_LAST_WIDGET = 2200,
    BOTTOM_VIEW_PANEL_X = 480,
    BOTTOM_VIEW_PANEL_Y = 392,
    BOTTOM_VIEW_PANEL_WIDTH = 143,
    BOTTOM_VIEW_PANEL_HEIGHT = 71,
    BOTTOM_VIEW_BACKGROUND_WIDTH = 159,
    BOTTOM_VIEW_BACKGROUND_ID = 2000,
    BOTTOM_VIEW_FOREGROUND_ID = 2001,
    BOTTOM_VIEW_TEXT_ID = 2100,
    BOTTOM_VIEW_TEXT_ID_2 = 2101,
    BOTTOM_VIEW_TEXT_BUFFER_SIZE = 30,
    BOTTOM_VIEW_COUNT_BUFFER_SIZE = 8,
    BOTTOM_VIEW_NO_ENEMY = -1,
    BOTTOM_VIEW_NO_ANIMATION = -1
};

enum AdventureArmyQuickViewConstant {
    ARMY_QUICK_ICON_SIZE = 32,
    ARMY_QUICK_ICON_BASELINE = 30,
    ARMY_QUICK_AREA_LEFT = 9,
    ARMY_QUICK_LABEL_HEIGHT = 12,
    ARMY_QUICK_FIRST_ROW_SHIFT = 22,
    ARMY_QUICK_SECOND_ROW_SHIFT = 44,
    ARMY_QUICK_FIRST_ROW_COUNT = 2,
    ARMY_QUICK_ONE_STACK = 1,
    ARMY_QUICK_TWO_STACKS = 2,
    ARMY_QUICK_THREE_STACKS = 3,
    ARMY_QUICK_FOUR_STACKS = 4,
    ARMY_QUICK_FIVE_STACK_X_SHIFT = 12,
    ARMY_QUICK_TEXT_WIDTH = 60,
    ARMY_QUICK_TEXT_X_ADJUSTMENT = 14,
    ARMY_QUICK_SIZE_LABEL_CAPACITY = 15,
    HERO_QUICK_ARMY_AREA_WIDTH = 160,
    HERO_QUICK_DETAILED_CREATURE_Y = 110,
    HERO_QUICK_DETAILED_LABEL_Y = 140,
    HERO_QUICK_VAGUE_FIRST_ROW_Y = 65,
    HERO_QUICK_DEFAULT_WINDOW_X = 302,
    HERO_QUICK_LOCATOR_ROW_HEIGHT = 30,
    HERO_QUICK_LOCATOR_BASE_Y = 111,
    HERO_QUICK_ARMY_LABEL_CAPACITY = 5,
    QUICK_VIEW_FLAG_COLOR_STRIDE = 2,
    TOWN_QUICK_ARMY_AREA_WIDTH = 192,
    TOWN_QUICK_FIRST_ROW_Y = 75,
    TOWN_QUICK_DEFAULT_WINDOW_X = 342,
    TOWN_QUICK_DEFAULT_WINDOW_Y = 176,
    TOWN_QUICK_TYPE_FRAME_BASE = 12,
    TOWN_QUICK_CASTLE_FRAME_OFFSET = 4,
    TOWN_QUICK_EMPTY_LABEL_CAPACITY = 20,
    TOWN_QUICK_EMPTY_LABEL_X = 0,
    TOWN_QUICK_EMPTY_LABEL_Y = 100,
    TOWN_QUICK_EMPTY_LABEL_WIDTH = 210
};

enum TownQuickInformation {
    TOWN_QUICK_INFORMATION_UNKNOWN = 0,
    TOWN_QUICK_INFORMATION_NAMES = 1,
    TOWN_QUICK_INFORMATION_ESTIMATES = 2,
    TOWN_QUICK_INFORMATION_EXACT = 3,
    TOWN_QUICK_INFORMATION_THIEVES_LAST = TOWN_QUICK_INFORMATION_ESTIMATES
};

enum AdventureQuickViewPlacementConstant {
    HERO_QUICK_VIEW_X_OFFSET = 73,
    HERO_QUICK_VIEW_Y_OFFSET = 65,
    HERO_QUICK_VIEW_WIDTH = 178,
    HERO_QUICK_VIEW_HEIGHT = 162,
    HERO_QUICK_VIEW_RIGHT_X = BORDER_MIDDLE_END - HERO_QUICK_VIEW_WIDTH,
    HERO_QUICK_VIEW_BOTTOM_Y = BORDER_MIDDLE_END - HERO_QUICK_VIEW_HEIGHT,
    TOWN_QUICK_VIEW_X_OFFSET = 89,
    TOWN_QUICK_VIEW_Y_OFFSET = 70,
    TOWN_QUICK_VIEW_WIDTH = 210,
    TOWN_QUICK_VIEW_HEIGHT = 172,
    TOWN_QUICK_VIEW_RIGHT_X = BORDER_MIDDLE_END - TOWN_QUICK_VIEW_WIDTH,
    TOWN_QUICK_VIEW_BOTTOM_Y = BORDER_MIDDLE_END - TOWN_QUICK_VIEW_HEIGHT,
    QUICK_INFO_X_OFFSET = 57,
    QUICK_INFO_Y_OFFSET = 25,
    QUICK_INFO_WIDTH = 146,
    QUICK_INFO_HEIGHT = 82,
    QUICK_INFO_RIGHT_X = BORDER_MIDDLE_END - QUICK_INFO_WIDTH,
    QUICK_INFO_BOTTOM_Y = BORDER_MIDDLE_END - QUICK_INFO_HEIGHT
};

enum AdventureDrawConstant {
    CELL_PIXELS = 32,
    CELL_PIXEL_SHIFT = 5,
    CELL_LAST_PIXEL = CELL_PIXELS - 1,
    STONE_TILE_NONE = -1,
    STONE_BORDER_LOW = -1,
    STONE_TILE_TOP_LEFT = 16,
    STONE_TILE_TOP_RIGHT = 17,
    STONE_TILE_BOTTOM_RIGHT = 18,
    STONE_TILE_BOTTOM_LEFT = 19,
    STONE_TILE_TOP_BASE = 20,
    STONE_TILE_RIGHT_BASE = 24,
    STONE_TILE_BOTTOM_BASE = 28,
    STONE_TILE_LEFT_BASE = 32,
    STONE_PATTERN_COORDINATE_SHIFT = 16,
    CLOUD_VARIANTS = 4,
    CLOUD_VARIANT_MASK = CLOUD_VARIANTS - 1,
    CLOUD_FLIPPED_FRAME_BASE = 100,
    CLOUD_X_ALTERNATE_FRAME_1 = 1,
    CLOUD_Y_ALTERNATE_FRAME = 3,
    CLOUD_X_ALTERNATE_FRAME_2 = 5,
    ROUTE_DRAW_Y_OFFSET = 2,
    MONSTER_FRAME_STRIDE = 7,
    MONSTER_FACING_FRAME_BASE = MONSTER_FRAME_STRIDE - 1,
    MONSTER_DRAW_Y_OFFSET = 5,
    HERO_BOAT_Y_OFFSET = -10
};

enum AdventureUpdateScreenConstant {
    UPDATE_ANIMATION_PHASES = 6,
    UPDATE_FRAME_CYCLE = 6,
    UPDATE_FRAME_STEP_1 = 1,
    UPDATE_FRAME_STEP_3 = 3,
    UPDATE_FRAME_STEP_5 = 5
};

enum AdventureAnimationPhaseIndex {
    ANIMATION_PHASE_COLUMN_0 = 0,
    ANIMATION_PHASE_COLUMN_1 = 1,
    ANIMATION_PHASE_COLUMN_2 = 2,
    ANIMATION_PHASE_COLUMN_3 = 3,
    ANIMATION_PHASE_COLUMN_0_INITIAL = 0,
    ANIMATION_PHASE_COLUMN_1_INITIAL = 1,
    ANIMATION_PHASE_COLUMN_2_INITIAL = 3,
    ANIMATION_PHASE_COLUMN_3_INITIAL = 5
};

enum ViewWorldConstant {
    VIEW_WORLD_CELL_PIXELS = 6,
    VIEW_WORLD_ORIGIN = 24,
    VIEW_WORLD_TILESET_COUNT = 16,
    VIEW_WORLD_GROUND_TILE_SHIFT = 2,
    VIEW_WORLD_GROUND_FLIPPED_FRAMES = 31,
    VIEW_WORLD_FLAG_CURRENT_HERO = 5,
    VIEW_WORLD_FLAG_ARTIFACT = 6,
    VIEW_WORLD_TOWN_FLAG_LEFT = 4,
    VIEW_WORLD_TOWN_FLAG_RIGHT = 3,
    VIEW_WORLD_RESOURCE_X_SHIFT = 3
};

enum AdventureRadarConstant {
    RADAR_LEFT = 480,
    RADAR_RIGHT = 624,
    RADAR_TOP = 16,
    RADAR_BOTTOM = 160,
    RADAR_SIZE = RADAR_RIGHT - RADAR_LEFT,
    RADAR_CELL_PIXELS = 2,
    RADAR_TERRAIN_SHADE = 3,
    RADAR_VIEWPORT_COLOR = 0xbe,
    RADAR_UNEXPLORED_COLOR = 0
};

enum AdventureTeleportConstant {
    TELEPORT_FIZZLE_TIME = 128,
    TELEPORT_REMOTE_FIZZLE_ADJUSTMENT = 64
};

enum AdventureSummonBoatConstant {
    SUMMON_RESTORE_MODE = 5,
    SUMMON_FIZZLE_X_OFFSET = 32,
    SUMMON_FIZZLE_Y_OFFSET = 16,
    SUMMON_FIZZLE_WIDTH = 96,
    SUMMON_FIZZLE_HEIGHT = 48,
    SUMMON_TARGET_X = 176,
    SUMMON_TARGET_Y = 192,
    SUMMON_TARGET_WIDTH = 128,
    SUMMON_TARGET_HEIGHT = 96
};

enum AdventurePuzzleViewConstant {
    PUZZLE_PIECE_COUNT = 48,
    PUZZLE_ALIGNMENT_DIVISOR = 3,
    PUZZLE_Y_ADJUST_X_FACTOR = 2,
    PUZZLE_Y_ADJUST_Y_FACTOR = 5,
    PUZZLE_PARITY_DIVISOR = 2,
    PUZZLE_FIZZLE_TIME = 220
};

enum AdventureStateConstant {
    FORCED_MUSIC_DELAY = 6000,
    CURSOR_SAMPLE_FAST_SET = 2,
    HIGH_MEMORY_BUFFER_DIVISOR = 100,
    SCROLL_Y = 195,
    SCROLL_LEFT_X = 540,
    SCROLL_RIGHT_X = 612,
    SCROLL_WIDTH = 8,
    SCROLL_HEIGHT = 17,
    SCROLL_ICON_FRAME = 4
};

enum AdventureEnvironmentSoundConstant {
    ENVIRONMENT_SOUND_FAR_DISTANCE = 127,
    ENVIRONMENT_SOUND_MAX_DISTANCE = 5,
    ENVIRONMENT_SOUND_FIRST_LAYER = 1,
    ENVIRONMENT_SOUND_LAYER_COUNT = 2,
    ENVIRONMENT_SOUND_EDGE_SPAN = 2,
    ENVIRONMENT_SOUND_RADIUS_COUNT = 4,
    ENVIRONMENT_SOUND_NO_SLOT = -1
};

enum AdventureComboDrawConstant {
    COMBO_CLEAR_BYTES = 256,
    COMBO_CLOUD_MARK = 10,
    COMBO_FRAME_LIMIT = 12,
    COMBO_FAR_NEIGHBOR_OFFSET = 2
};

enum AdventureCheatConstant {
    CHEAT_SEQUENCE_RADIX = 10,
    CHEAT_SEQUENCE_MODULUS = 1000000,
    CHEAT_REVEAL_MAP = 101495,
    CHEAT_RESOURCE_AMOUNT = 10,
    CHEAT_GOLD_AMOUNT = 1000,
    CHEAT_EXPERIENCE_AMOUNT = 800,
    CHEAT_SPELL_CHARGES = 5,
    CHEAT_MOBILITY = 2999,
    CHEAT_REVEAL_CENTER = 30,
    CHEAT_REVEAL_RADIUS = 100
};

enum AdventureEnemyTurnViewConstant {
    ENEMY_TURN_HOURGLASS_X = 493,
    ENEMY_TURN_HOURGLASS_Y = 403,
    ENEMY_TURN_HOURGLASS_WIDTH = 118,
    ENEMY_TURN_HOURGLASS_HEIGHT = 51,
    ENEMY_TURN_CREST_X = 495,
    ENEMY_TURN_ANIMATION_X = 559,
    ENEMY_TURN_ANIMATION_Y = 405,
    ENEMY_TURN_ANIMATION_WIDTH = 50,
    ENEMY_TURN_ANIMATION_HEIGHT = 47,
    ENEMY_TURN_CREST_ID = 2002,
    ENEMY_TURN_SAND_ID = 2003,
    ENEMY_TURN_PHASE_ID = 2004,
    ENEMY_TURN_BACKGROUND_Z = 1000,
    ENEMY_TURN_HOURGLASS_Z = 1010,
    ENEMY_TURN_SAND_Z = 1020,
    ENEMY_TURN_CREST_Z = 1030,
    ENEMY_TURN_PHASE_Z = 1040,
    ENEMY_TURN_SAND_FRAME_OFFSET = 11,
    ENEMY_TURN_SAND_FRAME_LIMIT = 20,
    ENEMY_TURN_SAND_RESTART_FRAME = 16,
    ENEMY_TURN_PHASE_FRAME_OFFSET = 1,
    ENEMY_TURN_CREST_SLOT = 0,
    ENEMY_TURN_SAND_SLOT = 1,
    ENEMY_TURN_PHASE_SLOT = 2,
    ENEMY_TURN_ANIMATION_DELAY = 300,
    ENEMY_TURN_PHASE_DELAY = 700
};

enum AdventureNewTurnViewConstant {
    NEW_TURN_DATE_TEXT_X = 479,
    NEW_TURN_WEEK_TEXT_Y = 421,
    NEW_TURN_DAY_TEXT_Y = 438,
    NEW_TURN_DATE_TEXT_WIDTH = 145,
    NEW_TURN_WEEK_TEXT_HEIGHT = 12,
    NEW_TURN_DAY_TEXT_HEIGHT = 25
};

enum AdventureResourceViewConstant {
    RESOURCE_VIEW_TEXT_BASE_Y = 395,
    RESOURCE_VIEW_MULTILINE_HEIGHT = 32,
    RESOURCE_VIEW_LINE_HEIGHT = 6,
    RESOURCE_VIEW_TEXT_HEIGHT = 36,
    RESOURCE_VIEW_GOLD_WIDTH = 76,
    RESOURCE_VIEW_GOLD_HEIGHT = 26,
    RESOURCE_VIEW_ICON_WIDTH = 38,
    RESOURCE_VIEW_ICON_HEIGHT = 32,
    RESOURCE_VIEW_ICON_BOTTOM = 463,
    RESOURCE_VIEW_ICON_BOTTOM_PADDING = 14,
    RESOURCE_VIEW_COUNT_X = 511,
    RESOURCE_VIEW_COUNT_Y = 450,
    RESOURCE_VIEW_COUNT_WIDTH = 80,
    RESOURCE_VIEW_COUNT_HEIGHT = 12
};

enum AdventureKingdomViewConstant {
    KINGDOM_VIEW_ENTRY_COUNT = 9,
    KINGDOM_VIEW_CASTLE_ENTRY = 7,
    KINGDOM_VIEW_TOWN_ENTRY = 8,
    KINGDOM_VIEW_ICON_X = 481,
    KINGDOM_VIEW_ICON_Y = 393,
    KINGDOM_VIEW_TEXT_X_BASE = 464,
    KINGDOM_VIEW_TEXT_Y_BASE = 392,
    KINGDOM_VIEW_TEXT_WIDTH = 32,
    KINGDOM_VIEW_TEXT_HEIGHT = 12,
    KINGDOM_VIEW_RESOURCE_TEXT_Y = 59,
    KINGDOM_VIEW_TOWN_TEXT_Y = 28,
    KINGDOM_VIEW_GOLD_TEXT_Y = KINGDOM_VIEW_TOWN_TEXT_Y,
    KINGDOM_VIEW_WOOD_TEXT_X = 15,
    KINGDOM_VIEW_MERCURY_TEXT_X = 38,
    KINGDOM_VIEW_ORE_TEXT_X = 61,
    KINGDOM_VIEW_SULFUR_TEXT_X = 85,
    KINGDOM_VIEW_CRYSTAL_TEXT_X = 109,
    KINGDOM_VIEW_GEMS_TEXT_X = 132,
    KINGDOM_VIEW_GOLD_TEXT_X = 123,
    KINGDOM_VIEW_CASTLE_TEXT_X = 27,
    KINGDOM_VIEW_VILLAGE_TEXT_X = 80
};

enum AdventureBottomHeroViewConstant {
    BOTTOM_HERO_LABEL_BYTES = 6,
    BOTTOM_HERO_SLOT_THIRD = 2,
    BOTTOM_HERO_SLOT_FOURTH = 3,
    BOTTOM_HERO_TWO_STACKS = 2,
    BOTTOM_HERO_FOUR_STACKS = 4,
    BOTTOM_HERO_ICON_WIDTH = 32,
    BOTTOM_HERO_ICON_HEIGHT = 28,
    BOTTOM_HERO_LABEL_HEIGHT = 12,
    BOTTOM_HERO_CHARACTER_WIDTH = 5,
    BOTTOM_HERO_FIRST_ICON_ID = 2002,
    BOTTOM_HERO_FIRST_TEXT_ID = 2101
};

enum AdventureScrollConstant {
    ADVMGR_VIEW_CELL_COUNT = 15,
    ADVMGR_VIEW_CENTER = 7,
    SCROLL_MIN_ORIGIN = -ADVMGR_VIEW_CENTER,
    SCROLL_MAX_ORIGIN = MAP_CELL_GRID_SIZE - ADVMGR_VIEW_CENTER - 1,
    SCROLL_TICK_INTERVAL = 70,
    HOVER_SCROLL_FRAME_FIRST = 32,
    HOVER_SCROLL_FRAME_END = 40
};

enum AdventurePanelDialogConstant {
    PANEL_CLOSE_WIDGET = DIALOG_BUTTON_0,
    PANEL_NO_HELP = -1,
    PANEL_VIEW_WORLD_HELP = 0,
    PANEL_VIEW_PUZZLE_HELP = 1,
    PANEL_CAST_SPELL_HELP = 2,
    PANEL_SEARCH_HELP = 3,
    PANEL_CLOSE_HELP = 4,
    PANEL_VIEW_WORLD = 1,
    PANEL_VIEW_PUZZLE = 2,
    PANEL_CAST_SPELL = 3,
    PANEL_SEARCH = 4
};

enum CloudNeighborMask {
    CLOUD_NORTH = 0x01,
    CLOUD_EAST = 0x02,
    CLOUD_SOUTH = 0x04,
    CLOUD_WEST = 0x08,
    CLOUD_NORTH_EAST = 0x10,
    CLOUD_SOUTH_EAST = 0x20,
    CLOUD_SOUTH_WEST = 0x40,
    CLOUD_NORTH_WEST = 0x80,
    CLOUD_EAST_EDGE = 0x32,
    CLOUD_SOUTH_EDGE = 0x64,
    CLOUD_NORTH_EDGE = 0x91,
    CLOUD_WEST_EDGE = 0xc8
};

enum AdventurePanelHelp {
    ADVENTURE_HELP_NONE = -1,
    ADVENTURE_HELP_NEXT_HERO = 0,
    ADVENTURE_HELP_FIRST = ADVENTURE_HELP_NEXT_HERO,
    ADVENTURE_HELP_CONTINUE_ROUTE = 1,
    ADVENTURE_HELP_OVERVIEW = 2,
    ADVENTURE_HELP_END_TURN = 3,
    ADVENTURE_HELP_ADVENTURE_OPTIONS = 4,
    ADVENTURE_HELP_GAME_OPTIONS = 5,
    ADVENTURE_HELP_COUNT = 6
};
extern char* gAdvMenuHelp[ADVENTURE_HELP_COUNT];

enum QuickViewWidget {
    QUICK_VIEW_NAME = 1,
    QUICK_VIEW_PORTRAIT = 2,
    QUICK_VIEW_STAT_FIRST = 3,
    QUICK_VIEW_FLAG = 8,
    QUICK_VIEW_AT_LOCATOR = -1,
    QUICK_VIEW_NO_LOCATOR = -1
};

enum ControlPanelDialogConstant {
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
    CONTROL_SCENARIO_INFO = 17,
    CONTROL_MUSIC_VOLUME_TEXT = 8,
    CONTROL_SOUND_VOLUME_TEXT = 9,
    CONTROL_WALK_SPEED_TEXT = 10,
    CONTROL_MUSIC_SOURCE_TEXT = 14,
    CONTROL_SHOW_ROUTE_TEXT = 15,
    CONTROL_SHOW_ENEMY_MOVES_TEXT = 16
};

enum ControlPanelFrame {
    CPANEL_FRAME_MUSIC_OFF = 10,
    CPANEL_FRAME_MUSIC_ON = 11,
    CPANEL_FRAME_SOUND_OFF = 12,
    CPANEL_FRAME_SOUND_ON = 13,
    CPANEL_FRAME_WALK_SPEED_FIRST = 14,
    CPANEL_FRAME_SHOW_ROUTE_FIRST = 21,
    CPANEL_FRAME_ENEMY_MOVES_FIRST = 23,
    CPANEL_FRAME_MUSIC_SOURCE_FIRST = 27
};

enum ControlPanelMusicLabel {
    CPANEL_MUSIC_LABEL_LOCAL = 0,
    CPANEL_MUSIC_LABEL_CD = 2
};

enum ControlPanelHelp {
    CPANEL_HELP_NONE = -1,
    CPANEL_HELP_NEW_GAME = 0,
    CPANEL_HELP_FIRST = CPANEL_HELP_NEW_GAME,
    CPANEL_HELP_LOAD_GAME = 1,
    CPANEL_HELP_QUIT = 2,
    CPANEL_HELP_CLOSE = 3,
    CPANEL_HELP_SAVE_GAME = 4,
    CPANEL_HELP_MUSIC_VOLUME = 5,
    CPANEL_HELP_SOUND_VOLUME = 6,
    CPANEL_HELP_WALK_SPEED = 7,
    CPANEL_HELP_MUSIC_SOURCE = 8,
    CPANEL_HELP_SHOW_ROUTE = 9,
    CPANEL_HELP_SHOW_ENEMY_MOVES = 10,
    CPANEL_HELP_SCENARIO_INFO = 11,
    CPANEL_HELP_COUNT = 12
};
extern char* gCPanelHelp[CPANEL_HELP_COUNT];

enum AdventureTravelSpellConstant {
    TRAVEL_DIALOG_REJECT = 0,
    TRAVEL_DIALOG_ACCEPT = 1,
    DIMENSION_DOOR_FIRST_BUTTON = ADVENTURE_CONTROL_MAP_VIEW,
    DIMENSION_DOOR_LAST_BUTTON = 11,
    TOWN_PORTAL_DISTANCE_LIMIT = 1000,
    TOWN_GATE_NO_TOWN = -1
};

enum CursorConstant {
    CURSOR_DRAW_X = 0xe0,
    CURSOR_DRAW_Y = 0xff,
    CURSOR_BOAT_DRAW_Y_ADJUST = 10,
    CURSOR_SHADOW_FLIP_X_ADJUST = 0x20,
    CURSOR_FLAG_FRAME_BASE = 0x38,
    CURSOR_FLAG_FRAME_CYCLE_MASK = 3,
    CURSOR_LAST_FRAME_COUNT = 8,
    CURSOR_TURN_FRAME_COUNT = 16,
    CURSOR_SLOW_TURN_MULTIPLIER = 3,
    CURSOR_MOVE_HALF_TILE_PIXELS = 16,
    CURSOR_DIAGONAL_DIRECTION_BIT = 1,
    CURSOR_CYCLE_STOPPED = 0,
    CURSOR_CYCLE_RUNNING = 1,
    SLOW_CURSOR_CYCLE_START = 2,
    SKIPPED_ANIMATION_FRAME_EARLY = 1,
    SKIPPED_ANIMATION_FRAME = 4,
    FOOTSTEP_ANIMATION_FRAME = 3,
    DIRECTION_HALF_COUNT = 4,
    TURN_FRAME_MULTIPLIER = 2,
    MOVE_TILE_HALF_COUNT = 2
};

#endif

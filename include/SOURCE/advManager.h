#ifndef HOMM1_SOURCE_ADVMANAGER_H
#define HOMM1_SOURCE_ADVMANAGER_H

#include <BASE/audioTypes.h>
#include <BASE/baseManager.h>
#include <BASE/message.h>
#include <Domains.h>
#include <H1/Macros.h>
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

// forward declarations:
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

H1_ENUM_CONST_BEGIN(AdventureManagerStorageConstant)
    ADVMGR_BOTTOM_VIEW_WIDGET_COUNT = 12,
    // m_bottomViewPrimaryWidgets slots: the stone backdrop, the view's
    // foreground icon, then its further icons; the secondary (text) array's
    // army/count labels start at HERO_TEXT_FIRST.
    ADVMGR_BOTTOM_VIEW_BACKGROUND = 0,
    ADVMGR_BOTTOM_VIEW_FOREGROUND = 1,
    ADVMGR_BOTTOM_VIEW_ICON_FIRST = 2,
    ADVMGR_BOTTOM_VIEW_HERO_TEXT_FIRST = 1,
    ADVMGR_OBJECT_ICON_COUNT = 21,
    ADVMGR_PANEL_ICON_COUNT = 5,
    ADVMGR_ANIMATION_PHASE_COUNT = 4,
    ADVMGR_HERO_ICON_COUNT = 5,
    ADVMGR_PLAYER_COLOR_COUNT = 4,
    ADVMGR_ACTIVE_SOUND_COUNT = 8,
    ADVMGR_ENVIRONMENT_SOUND_COUNT = 22,
    ADVMGR_CURSOR_SAMPLE_COUNT = 7
H1_ENUM_CONST_END(AdventureManagerStorageConstant)

// The adventure screen's animation clock (KB.h ADVENTURE_FRAME_TIMER_SLOT)
// re-armed TIMER_DELAY ms ahead (advManager::Open/Main/UpdateScreen,
// DimensionDoorHandler, philAI's CheckDoMain).
H1_ENUM_CONST_BEGIN(AdventureFrameTimerConstant)
    TIMER_DELAY = 120
H1_ENUM_CONST_END(AdventureFrameTimerConstant)

// m_combatMonsterX/Y: the map cell of the monster CombatMonsterEvent turns to
// face the attacker (DrawCell draws it facing); the constructor starts it at
// NONE (-99, off every drawable cell) and CombatMonsterEvent clears the x back
// to CLEARED (-1) after the redraw. A combatX of AT_EVENT fights the monster
// on the event cell itself, with no turn to face the attacker.
H1_ENUM_CONST_BEGIN(AdventureCombatMonsterCellConstant)
    COMBAT_MONSTER_CELL_NONE = -99,
    COMBAT_MONSTER_CELL_CLEARED = -1,
    COMBAT_MONSTER_CELL_AT_EVENT = -1
H1_ENUM_CONST_END(AdventureCombatMonsterCellConstant)

// m_hoverCellX/m_hoverCellY before the mouse hovers a view cell.
H1_ENUM_CONST_BEGIN(AdventureCursorConstant)
    CURSOR_INVALID_POSITION = -1
H1_ENUM_CONST_END(AdventureCursorConstant)

// advManager::GiveArtifact's result when every artifact slot is taken.
H1_ENUM_CONST_BEGIN(AdventureGiveArtifactConstant)
    GIVE_ARTIFACT_NO_SLOT = -1
H1_ENUM_CONST_END(AdventureGiveArtifactConstant)

// Hero sprite frame codes (m_cursorFrame, DrawCell's map heroes): bit 7 draws
// the frame mirrored, the low seven bits index the sprite.
H1_ENUM_CONST_BEGIN(AdventureHeroFrameConstant)
    HERO_FRAME_MIRROR_FLAG = 0x80,
    HERO_FRAME_INDEX_MASK = 0x7f
H1_ENUM_CONST_END(AdventureHeroFrameConstant)

// m_heroIcons slots and m_cursorType: the four hero-class sprites (the
// constructor loads kngt32/barb32/sorc32/wrlk32.icn; MobilizeCurrHero and
// DoEvent store the hero class) and the boat (boat32.icn; set on boarding,
// tested for water moves and shadows).
H1_ENUM_BEGIN(AdventureHeroIcon)
    ADVMGR_HERO_ICON_KNIGHT = 0,
    ADVMGR_HERO_ICON_BARBARIAN = 1,
    ADVMGR_HERO_ICON_SORCERESS = 2,
    ADVMGR_HERO_ICON_WARLOCK = 3,
    ADVMGR_HERO_ICON_CLASS_END = 4,
    ADVMGR_HERO_ICON_BOAT = 4
H1_ENUM_END(AdventureHeroIcon)

// m_pendingCommand: the action ProcessHover/ProcessSelect queue and
// advManager::DoAdvCommand runs.
H1_ENUM_BEGIN(AdventureCommand)
    ADVMGR_COMMAND_NONE = -1,
    ADVMGR_COMMAND_MOVE_TO = 1,
    ADVMGR_COMMAND_HERO_VIEW = 2,
    ADVMGR_COMMAND_TOWN_VIEW = 3,
    ADVMGR_COMMAND_SELECT_HERO = 4,
    ADVMGR_COMMAND_SELECT_TOWN = 5,
    ADVMGR_COMMAND_OCCUPIED_TOWN_VIEW = 6,
    ADVMGR_COMMAND_CONTINUE_ROUTE = 7
H1_ENUM_END(AdventureCommand)

// CastSpell charges Dimension Door and Town Gate this much mobility, and
// philAI::DoDimensionDoor the same for the AI's jump.
H1_ENUM_CONST_BEGIN(AdventureSpellConstant)
    SPELL_TRAVEL_MOBILITY_COST = 12
H1_ENUM_CONST_END(AdventureSpellConstant)

// ProcessSearch digs at (x, y), or at the view centre when x is
// VIEW_CENTER (the D key, the menu and philAI pass it for both); it marks a
// dug cell with obj32-07.icn frame 1 as a shadow-only object, which DrawCell
// hides on the puzzle map.
H1_ENUM_CONST_BEGIN(AdventureSearchConstant)
    ADVMGR_SEARCH_VIEW_CENTER = -1,
    DIG_HOLE_FRAME = 1
H1_ENUM_CONST_END(AdventureSearchConstant)

// Hero/town locator paging: VISIBLE_COUNT rows show at once; a column with
// fewer than PAGE_THRESHOLD entries has no pages (advManager's knobs and
// town::Deallocate's page clamp). UpdateHeroLocator's SLOT_CURRENT_HERO
// redraws whichever visible slot shows the current hero.
H1_ENUM_CONST_BEGIN(AdventureLocatorConstant)
    LOCATOR_SLOT_CURRENT_HERO = -1,
    LOCATOR_VISIBLE_COUNT = 4,
    LOCATOR_PAGE_THRESHOLD = 5,
    LOCATOR_PAGE_DENOMINATOR_OFFSET = 4,
    LOCATOR_SCROLL_NO_PAGES_Y = 232,
    // The knobs slide from SCROLL_BASE_Y over the hero (73) or town (74)
    // span; a click on the track maps the 92-pixel strip from 194 to a page.
    LOCATOR_SCROLL_BASE_Y = 195,
    LOCATOR_HERO_SCROLL_SPAN = 73,
    LOCATOR_TOWN_SCROLL_SPAN = 74,
    LOCATOR_SCROLL_MOUSE_BASE_Y = LOCATOR_SCROLL_BASE_Y - 1,
    LOCATOR_SCROLL_MOUSE_SPAN = 92
H1_ENUM_CONST_END(AdventureLocatorConstant)

// Adventure-window widget ids handled by advManager::Main,
// ProcessSelect/DeSelect/Hover: the six panel buttons, radar, map view and
// the hero/town locator columns.
H1_ENUM_ID_BEGIN(AdventureControl)
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
    ADVENTURE_CONTROL_HERO_LOCATOR_4 = 126 H1_ENUM_ID_END(AdventureControl)

    // The six panel buttons (ADVENTURE_CONTROL_NEXT_HERO..GAME_OPTIONS) that
    // DisableButtons/EnableButtons and game's Show/CancelComputerScreen dim.
    H1_ENUM_CONST_BEGIN(AdventurePanelButtonConstant)
    ADVMGR_PANEL_BUTTON_FIRST = 1,
    ADVMGR_PANEL_BUTTON_LAST = 6
H1_ENUM_CONST_END(AdventurePanelButtonConstant)

// Unconditional six-button enable/disable broadcast; the window expression
// is re-evaluated for every broadcast.
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

// m_activeSounds: a playing map sound and its nearest ring distance.
struct adventureSoundCell {
    i32 soundId;
    i32 distance;
};

// DrawCell's layers.
H1_ENUM_FLAGS_BEGIN(AdventureDrawMask, i8)
    ADVMGR_DRAW_GROUND = 0x01,
    ADVMGR_DRAW_OBJECT = 0x02,
    ADVMGR_DRAW_OVERLAY = 0x04,
    ADVMGR_DRAW_HERO = 0x08,
    ADVMGR_DRAW_CLOUD = 0x20
H1_ENUM_FLAGS_END(AdventureDrawMask)

// Retail constructor, Open and InitMainClasses' 0x260-byte allocation fix
// this packed layout after the 0x30-byte baseManager prefix.
#pragma pack(push, 1)
class advManager : public baseManager {
public:
    H1_ENUM_STORAGE(AdventureCommand, i8) m_pendingCommand;
    class widget* m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_WIDGET_COUNT];
    class widget* m_bottomViewSecondaryWidgets[ADVMGR_BOTTOM_VIEW_WIDGET_COUNT];
    class heroWindow* m_adventureWindow;
    // ShowRoute clears 72*72 bytes and stores signed route frames.
    i8* m_routeMap;
    b8 m_routeShown;
    H1_ENUM_STORAGE(TerrainType, i8) m_currentTerrain;
    char m_unused9b[4];
    class mapCell (*m_mapData)[MAP_CELL_GRID_SIZE];
    class iconWidget* m_scrollLeftButton;
    class iconWidget* m_scrollRightButton;
    // Open adds these five panel backdrops to the adventure window.
    class backdropWidget* m_panelBackdrops[ADVMGR_PANEL_ICON_COUNT];
    u8* m_adventureBorder;
    char m_unusedc3[4];
    class tileset* m_groundTiles;
    class tileset* m_cloudTiles;
    class tileset* m_stoneTiles;
    H1_ENUM_ARRAY(class icon*, m_objectIcons, MapTileset, ADVMGR_OBJECT_ICON_COUNT);
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
    H1_ENUM_ARRAY(class icon*, m_heroIcons, AdventureHeroIcon, ADVMGR_HERO_ICON_COUNT);
    class icon* m_shadowIcon;
    H1_ENUM_ARRAY(class icon*, m_flagIcons, PlayerColor, ADVMGR_PLAYER_COLOR_COUNT);
    H1_ENUM_ARRAY(class icon*, m_boatFlagIcons, PlayerColor, ADVMGR_PLAYER_COLOR_COUNT);
    b8 m_cursorActive;
    b8 m_drawHeroShadows;
    H1_ENUM_STORAGE(AdventureHeroIcon, u8) m_cursorType;
    H1_ENUM_STORAGE(MapDirection, i8) m_cursorDirection;
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
    H1_ENUM_ARRAY(class sample*, m_cursorSamples, TerrainType, ADVMGR_CURSOR_SAMPLE_COUNT);
    b8 m_identifyHeroActive;
    b8 m_heroesLogoShown;
    // Main drops message types outside this mask (Open sets 0x32f).
    i16 m_messageTypeMask;
    // --- constructors ---
    advManager(void);
    ~advManager();
    // --- virtual methods (vtable order) ---
    virtual H1_ENUM_RETURN(BaseManagerStatus, i16) Open(i16 id) OVERRIDE;
    virtual void Close(void) OVERRIDE;
    virtual H1_ENUM_RETURN(MessageDispatchResult, i16) Main(struct tag_message& message) OVERRIDE;
    // --- methods ---
    void StartCursor(H1_ENUM_PARAM(MapDirection, i8) direction);
    void StopCursor(b8 stopSound);
    void DrawCursor(void);
    i16 GetCursorBaseFrame(H1_ENUM_PARAM(MapDirection, i16) direction);
    void TurnTo(H1_ENUM_PARAM(MapDirection, i8) direction);
    b32 GetMoveShowIt(H1_ENUM_PARAM(MapDirection, i8) direction);
    class mapCell* MoveHero(
        H1_ENUM_PARAM(MapDirection, i8) direction,
        b8 stopAfterMove,
        i32* eventX,
        i32* eventY,
        b32* outOfMobility,
        b8 processEvent,
        b8* adjacentMonster
    );
    void CheckAdjacentMon(b8* adjacentMonster);
    i16 ValidMoveWithEvent(class hero* movingHero, H1_ENUM_PARAM(MapDirection, i16) direction);
    i16 ValidMove(H1_ENUM_PARAM(MapDirection, i16) direction);
    void MoveOrigin(i16 directionX, i16 directionY);
    void ViewWorld(H1_ENUM_PARAM(SpellType, i8) spellType, b8 drawAllObjects, b8 drawAllTerrains);
    void GetCursorSampleSet(i32 sampleSet);
    class mapCell* DoAdvCommand(void);
    i32 GetCommandTargetX(void) {
        return m_commandTargetX;
    }
    i32 GetCommandTargetY(void) {
        return m_commandTargetY;
    }
    void Reseed(i32, i32);
    H1_ENUM_RETURN(MessageDispatchResult, i32) ProcessSelect(struct tag_message* message, class mapCell** eventCell);
    H1_ENUM_RETURN(MessageDispatchResult, i32) ProcessDeSelect(struct tag_message* message, i32* result, class mapCell** eventCell);
    b32 ProcessSearch(i32 x, i32 y);
    H1_ENUM_RETURN(MessageDispatchResult, i32) ProcessHover(struct tag_message* message);
    void UpdateScreen(b8 cursorUpdate, b8 forceUpdate);
    void CompleteDraw(i16 originX, i16 originY, b32 forceDraw);
    void CompleteDraw(b32 forceDraw);
    i32 GetCloudLookup(i32 x, i32 y);
    void DrawCell(
        i16 mapX,
        i16 mapY,
        i16 screenX,
        i16 screenY,
        H1_ENUM_PARAM(AdventureDrawMask, i8) drawMask,
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
    char* GetArmySizeName(i16 armySize, H1_ENUM_PARAM(ArmySizeNameVariant, i8) grammar);
    void TownQuickView(i8 townId, i8 locatorSlot, i16 windowX, i16 windowY);
    void RedrawAdvScreen(b32 update);
    void GiveTakeArtifactStat(
        class hero* targetHero,
        H1_ENUM_PARAM(ArtifactType, i8) artifact,
        H1_ENUM_PARAM(EventArtifactStat, i8) take
    );
    void DeactivateCurrTown(void);
    void DeactivateCurrHero(void);
    void MobilizeCurrHero(b32 update);
    void DemobilizeCurrHero(void);
    void SetTownContext(i8 townId);
    void SetHeroContext(i8 heroId, b8 update);
    void DoHeroKnob(void);
    void DoTownKnob(void);
    void CastSpell(H1_ENUM_PARAM(SpellType, i8) spell);
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
    void ScreenScroll(H1_ENUM_PARAM(MapDirection, i8) direction, b32 updatePointer);
    void CheckScreenScroll(void);
    b32 MouseInScrollZone(void);
    void SetInitialMapOrigin(void);
    void LoadRemote(void);
    RemoteMessage* CheckHandleNet(void);
    H1_ENUM_RETURN(MessageDispatchResult, i16) CheckHandleNetPlayerWait(struct tag_message& message, b8 doMain);
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
    void EventSound(H1_ENUM_PARAM(MapObjectType, i16) eventType, i16 eventData);
    void EventWindow(
        H1_ENUM_PARAM(MapEventTextId, i16) eventId,
        H1_ENUM_PARAM(NormalDialogType, i32) buttons,
        char* text,
        H1_ENUM_PARAM(NormalDialogResourceType, i32) type1,
        i32 value1,
        H1_ENUM_PARAM(NormalDialogResourceType, i32) type2,
        i32 value2,
        H1_ENUM_PARAM(NormalDialogOrText, i32) showOrText
    );
    H1_ENUM_RETURN(ArtifactType, i32) GiveRandomArtifact(class hero* eventHero);
    i32 GiveExperience(class hero* eventHero, i32 experience, b8 checkLevel);
    void GiveResource(class hero* eventHero, H1_ENUM_PARAM(ResourceType, i8) resource, i16 amount);
    i16 GiveArtifact(class hero* eventHero, H1_ENUM_PARAM(ArtifactType, i8) artifact);
    void RecruitEvent(
        class hero* eventHero,
        H1_ENUM_PARAM(CreatureType, i32) creatureType,
        class mapCell* cell
    );
    b8 GhostEvent(
        class hero* eventHero,
        class mapCell* cell,
        H1_ENUM_PARAM(MapEventTextId, i32) textId,
        i32 x,
        i32 y
    );
    void HouseEvent(class hero* eventHero, class mapCell* cell);
    H1_ENUM_RETURN(CombatSide, i8) CombatMonsterEvent(
        class hero* eventHero,
        H1_ENUM_PARAM(CreatureType, i8) monsterType,
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
    void FizzleCenter(H1_ENUM_PARAM(EventFizzleType, i32) fizzleType);
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
    H1_ENUM_RETURN(CombatSide, i32) DoCombat(
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
        i8 retreatWin,
        i8 combatSurrender
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
        i8* retreatWin,
        i8* combatSurrender
    );
};
#pragma pack(pop)

H1_ENUM_RETURN(MessageDispatchResult, i16) APanelHandler(struct tag_message& message);
void UpdateCPanel(b8 initialDraw);
i8 SaveGame(void);
H1_ENUM_RETURN(MessageDispatchResult, i16) CPanelHandler(struct tag_message& message);

// gForceSwitchMusic: the tick a network turn hand-over forced a music
// switch, or IDLE when none is pending (advManager::Main, game::NewDay).
H1_ENUM_CONST_BEGIN(ForcedMusicConstant)
    FORCED_MUSIC_IDLE = -1
H1_ENUM_CONST_END(ForcedMusicConstant)
extern i32 gLastScrollTime;
extern b32 gForceUpdate;
// The adventure music for a terrain: MusicTrack's first seven tracks are
// the TerrainType themes in terrain order, so m_currentTerrain plays as its
// own track (advManager, game, hero and KB restart the terrain music).
#if H1_STRICT_DOMAINS
inline constexpr MusicTrack TerrainMusicTrack(TerrainType terrain) {
    return static_cast<MusicTrack>(static_cast<int>(terrain));
}
#define TERRAIN_MUSIC_TRACK(terrain) TerrainMusicTrack(terrain)
#else
#define TERRAIN_MUSIC_TRACK(terrain) (terrain)
#endif

// The adventure screen's bottom-right panel: gCurBottomView is the view
// UpdBottomView last drew, gBottomViewOverride (defined in KB) a temporary one that
// wins until its end time: the new-turn/kingdom toggle, KB's BVResMsg
// resource message, and game's DISABLED hold while the AI moves.
H1_ENUM_BEGIN(BottomViewMode)
    BOTTOM_VIEW_NONE = 0,
    BOTTOM_VIEW_NEW_TURN = 1,
    BOTTOM_VIEW_KINGDOM = 2,
    BOTTOM_VIEW_HERO = 3,
    BOTTOM_VIEW_ENEMY_TURN = 4,
    BOTTOM_VIEW_RESOURCE = 5,
    BOTTOM_VIEW_OVERRIDE_DISABLED = 6
H1_ENUM_END(BottomViewMode)
extern H1_ENUM_STORAGE(BottomViewMode, i32) gCurBottomView;
#define gBottomViewOverride giBottomViewOverride // spelling fixes .bss order
extern H1_ENUM_STORAGE(BottomViewMode, i32) gBottomViewOverride;
extern i32 gCurBottomViewEnemy;
#define gLastAnimFrame iLastAnimFrame // spelling fixes .bss order
extern i32 gLastAnimFrame;
// UpdBottomViewEnemyTurn's hourglass animation clocks and frames.
extern i32 gSandAnim;
extern i32 gLastHourGlassPhase;
extern i32 gLastHourGlassUpdateTime;
// Volume per environment-sound distance step.
extern const i32 gEnvironmentVolume[];
// gLimitUpdMinX with no pending limit box (UpdateScreen then redraws the
// whole viewport), and m_previousCursorMapX/Y with no hero-cursor cell to
// clear.
H1_ENUM_CONST_BEGIN(AdventureUpdateLimitConstant)
    UPDATE_NONE = -1,
    CURSOR_CELL_NONE = -1
H1_ENUM_CONST_END(AdventureUpdateLimitConstant)

extern i32 gLimitUpdMinX;
#define gLimitUpdMinY giLimitUpdMinY // spelling fixes .bss order
extern i32 gLimitUpdMinY;
extern i32 gLimitUpdMaxX;
#define gLimitUpdMaxY giLimitUpdMaxY // spelling fixes .bss order
extern i32 gLimitUpdMaxY;
extern class heroWindow* gAdventurePanel;
#define gPrefsChanged bPrefsChanged // spelling fixes .bss order
extern b8 gPrefsChanged;
#define gFreshSave gSaveClean // spelling fixes .bss order
extern b8 gFreshSave;
// ComboDraw's per-view-cell redraw marks and its animation frame clock.
#define gComboDraw bComboDraw // spelling fixes .bss order
extern i8 gComboDraw[][17];
// The cell whose trigger MoveHero reports, handed from DoAdvCommand's walk
// to DoEvent.
#define gTriggerX TrigX // spelling fixes .bss order
extern i32 gTriggerX;
#define gTriggerY TrigY // spelling fixes .bss order
extern i32 gTriggerY;
// CURSOR globals: the footstep and alternate-frame flags, and the hero cursor
// state DrawCursor saves and restores around gDrawSavedCursor.
extern b8 gMoveSoundMade;

#define gEveryOther EveryOther // spelling fixes .bss order
extern i8 gEveryOther;
#define gSavedCursorDirection S1cursorDirection // spelling fixes .bss order
extern H1_ENUM_STORAGE(MapDirection, i8) gSavedCursorDirection;
#define gSavedCursorBaseFrame S1cursorBaseFrame // spelling fixes .bss order
extern i16 gSavedCursorBaseFrame;
#define gSavedCursorFrameCount S1cursorFrameCount // spelling fixes .bss order
extern i16 gSavedCursorFrameCount;
#define gSavedCursorCycle S1cursorCycle // spelling fixes .bss order
extern i16 gSavedCursorCycle;
#define gSavedCursorTurning S1cursorTurning // spelling fixes .bss order
extern i16 gSavedCursorTurning;
extern H1_ENUM_ARRAY(i16, gStepDelay, WalkSpeed, WALK_SPEED_COUNT);
// MoveHero's pixels per walk step by speed and the step offsets.
extern H1_ENUM_ARRAY(i16, gPixelsPerStep, WalkSpeed, WALK_SPEED_COUNT);
#define gStepScrollStart startVals // spelling fixes .bss order
extern i16 gStepScrollStart[];
#define gFrameStep giFrameStep // spelling fixes .bss order
extern i32 gFrameStep;

struct SMapChange {
    char _pad[64];
};
#define gArmySizeName cArmySizeName // spelling fixes .bss order
extern char gArmySizeName[];
extern i32 gCurHourGlassPhase;

// Moved from ADVMGR.cpp.
H1_ENUM_CONST_BEGIN(AdventureButtonConstant)
    BUTTON_BROADCAST_ARG = 1,
    PANEL_CONTINUE_ROUTE = 2
H1_ENUM_CONST_END(AdventureButtonConstant)

H1_ENUM_CONST_BEGIN(AdventureScreenConstant)
    SCROLL_BORDER = 16
H1_ENUM_CONST_END(AdventureScreenConstant)

// The adventure map view is ADVENTURE_VIEWPORT_EXTENT pixels square at the
// screen origin; its frame covers BORDER_EDGE_SIZE pixels on each side, so
// the visible map is the inner box [BORDER_EDGE_SIZE, BORDER_MIDDLE_END):
// UpdateScreen's region, the quick views' and SummonBoat's clamps, the combo
// draw's update limits, and the saved frame strips (BORDER_*_BYTES).
H1_ENUM_CONST_BEGIN(AdventureBorderConstant)
    ADVENTURE_VIEWPORT_EXTENT = 480,
    BORDER_EDGE_SIZE = 16,
    BORDER_SIDE_BYTES = 16,
    BORDER_SAVED_SIDE_BYTES = 32,
    BORDER_MIDDLE_END = ADVENTURE_VIEWPORT_EXTENT - BORDER_EDGE_SIZE,
    ADVENTURE_VIEWPORT_INNER_SIZE = BORDER_MIDDLE_END - BORDER_EDGE_SIZE,
    BORDER_BUFFER_SIZE = 0x7400
H1_ENUM_CONST_END(AdventureBorderConstant)

// adv_wind.bin hero locator rows: seven widgets per slot from
// HERO_LOCATOR_WIDGET_BASE + slot * HERO_LOCATOR_WIDGET_STRIDE (UpdateHeroLocator);
// +1 is the mobility bar, +2 the portrait, +5 the clickable
// ADVENTURE_CONTROL_HERO_LOCATOR_n and +6 the selection frame. The town
// column's selection frames start at TOWN_LOCATOR_HIGHLIGHT_FIRST; the
// selected entry's frame takes LOCATOR_HIGHLIGHT_COLOR.
H1_ENUM_CONST_BEGIN(AdventureLocatorWidget)
    HERO_LOCATOR_WIDGET_BASE = 100,
    HERO_LOCATOR_WIDGET_STRIDE = 7,
    HERO_LOCATOR_MOBILITY = 1,
    HERO_LOCATOR_PORTRAIT = 2,
    HERO_LOCATOR_BUTTON = 5,
    HERO_LOCATOR_HIGHLIGHT = 6,
    TOWN_LOCATOR_HIGHLIGHT_FIRST = 32,
    LOCATOR_HIGHLIGHT_COLOR = 0xc5
H1_ENUM_CONST_END(AdventureLocatorWidget)

// locators.icn frames: the empty hero slots (one per slot), the empty town
// slots from EMPTY_TOWN_FIRST, the occupied hero frame, and the town frames
// by town type from TOWN_FIRST, CASTLE_OFFSET further on once it has a castle.
// m_routeMap while a route is shown (ShowRoute, DrawCell): NONE off the
// route, else a 1-based route.icn frame (FRAME_MASK) with FLIPPED mirroring it. The last step is
// the DESTINATION mark; steps the hero reaches today move REACHABLE_OFFSET
// frames on to the second arrow set.
H1_ENUM_CONST_BEGIN(AdventureRouteCell)
    ROUTE_CELL_NONE = 0,
    ROUTE_CELL_FRAME_MASK = 0x1f,
    ROUTE_CELL_FLIPPED = 0x20,
    ROUTE_CELL_DESTINATION = 14,
    ROUTE_CELL_REACHABLE_OFFSET = 14
H1_ENUM_CONST_END(AdventureRouteCell)

// The route-overlay byte at (column, row) of this->m_routeMap, indexed
// row-major as row * size + column.
#define ADVMGR_ROUTE_AT(column, row) (*(m_routeMap + (column) + (row) * MAP_CELL_GRID_SIZE))

H1_ENUM_CONST_BEGIN(AdventureLocatorFrame)
    LOCATOR_FRAME_EMPTY_TOWN_FIRST = 4,
    LOCATOR_FRAME_HERO = 8,
    LOCATOR_FRAME_TOWN_FIRST = 12,
    LOCATOR_FRAME_CASTLE_OFFSET = 4
H1_ENUM_CONST_END(AdventureLocatorFrame)

H1_ENUM_CONST_BEGIN(BottomViewPanelConstant)
    BOTTOM_VIEW_DRAW_FIRST_WIDGET = 2000,
    BOTTOM_VIEW_DRAW_LAST_WIDGET = 2200,
    BOTTOM_VIEW_PANEL_X = 480,
    BOTTOM_VIEW_PANEL_Y = 392,
    BOTTOM_VIEW_PANEL_WIDTH = 143,
    BOTTOM_VIEW_PANEL_HEIGHT = 71,
    // The stone backdrop is wider than the panel; the backdrop/foreground
    // icons and the text widgets take these ids; the text and count buffers
    // are malloc'd at these sizes.
    BOTTOM_VIEW_BACKGROUND_WIDTH = 159,
    BOTTOM_VIEW_BACKGROUND_ID = 2000,
    BOTTOM_VIEW_FOREGROUND_ID = 2001,
    BOTTOM_VIEW_TEXT_ID = 2100,
    BOTTOM_VIEW_TEXT_ID_2 = 2101,
    BOTTOM_VIEW_TEXT_BUFFER_SIZE = 30,
    BOTTOM_VIEW_COUNT_BUFFER_SIZE = 8,
    // No enemy turn drawn yet, no hourglass frame shown.
    BOTTOM_VIEW_NO_ENEMY = -1,
    BOTTOM_VIEW_NO_ANIMATION = -1
H1_ENUM_CONST_END(BottomViewPanelConstant)

// UpdBottomViewEnemyTurn's hourglass panel: the hourglass, running-sand and
// crest icons, their widget ids and z-orders, the sand frame cycle and the
// animation delays.
// The quick views' army rows: 32-pixel creature icons over a 12-pixel label
// 30 below, rows of up to three (the vague layout puts two over three, the
// first row 22 lower when there is only one row, the second 44 lower), a
// five-stack first row nudged 12 pixels apart, label buffers, the windows'
// default positions, and how much a town view reveals (thieves' guilds).
H1_ENUM_CONST_BEGIN(AdventureArmyQuickViewConstant)
    ARMY_QUICK_ICON_SIZE = 32,
    ARMY_QUICK_ICON_BASELINE = 30,
    ARMY_QUICK_AREA_LEFT = 9,
    ARMY_QUICK_LABEL_HEIGHT = 12,
    ARMY_QUICK_FIRST_ROW_SHIFT = 22,
    ARMY_QUICK_SECOND_ROW_SHIFT = 44,
    ARMY_QUICK_FIRST_ROW_COUNT = 2,
    // Stack counts the layouts distinguish: up to three fit one row; four
    // split two over two; five two over three.
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
H1_ENUM_CONST_END(AdventureArmyQuickViewConstant)

// TownQuickView's detail level: the owner sees exact counts; others see as
// much as their thieves' guilds reveal, capped at size names.
H1_ENUM_BEGIN(TownQuickInformation)
    TOWN_QUICK_INFORMATION_UNKNOWN = 0,
    TOWN_QUICK_INFORMATION_NAMES = 1,
    TOWN_QUICK_INFORMATION_ESTIMATES = 2,
    TOWN_QUICK_INFORMATION_EXACT = 3,
    // The most a rival's thieves' guilds can reveal.
    TOWN_QUICK_INFORMATION_THIEVES_LAST = TOWN_QUICK_INFORMATION_ESTIMATES
H1_ENUM_END(TownQuickInformation)

// Right-click quick views over the map: the window is offset from the
// clicked cell and clamped inside the viewport's inner box (the right/bottom
// limits are the box edge minus the size).
H1_ENUM_CONST_BEGIN(AdventureQuickViewPlacementConstant)
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
H1_ENUM_CONST_END(AdventureQuickViewPlacementConstant)

// DrawCell's sprite layout: cell pixels, the stone border tiles around the
// map (corners, then four-tile runs per side picked by the coordinate's low
// bits, the inner pattern offset by 16), the cloud variants and
// flipped/alternate frames, the route arrow's y offset, minimon frames (seven
// per creature, the facing frame last), the boat's y offset and the hero
// frame's mirror bit.
H1_ENUM_CONST_BEGIN(AdventureDrawConstant)
    CELL_PIXELS = 32,
    CELL_PIXEL_SHIFT = 5,
    CELL_LAST_PIXEL = CELL_PIXELS - 1,
    STONE_TILE_NONE = -1,
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
H1_ENUM_CONST_END(AdventureDrawConstant)

// UpdateScreen's animation clock and dirty box: m_animationFrame cycles through
// 6 steps and the columns start at 0/1/3/5; no limit box means the whole
// 448-pixel viewport at 16,16; odd steps advance columns 1 and 3, even ones
// 0 and 2, each modulo 6 frames.
H1_ENUM_CONST_BEGIN(AdventureUpdateScreenConstant)
    UPDATE_ANIMATION_PHASES = 6,
    UPDATE_FRAME_CYCLE = 6,
    // The odd steps, which advance columns 1 and 3.
    UPDATE_FRAME_STEP_1 = 1,
    UPDATE_FRAME_STEP_3 = 3,
    UPDATE_FRAME_STEP_5 = 5
H1_ENUM_CONST_END(AdventureUpdateScreenConstant)

H1_ENUM_CONST_BEGIN(AdventureAnimationPhaseIndex)
    ANIMATION_PHASE_COLUMN_0 = 0,
    ANIMATION_PHASE_COLUMN_1 = 1,
    ANIMATION_PHASE_COLUMN_2 = 2,
    ANIMATION_PHASE_COLUMN_3 = 3,
    ANIMATION_PHASE_COLUMN_0_INITIAL = 0,
    ANIMATION_PHASE_COLUMN_1_INITIAL = 1,
    ANIMATION_PHASE_COLUMN_2_INITIAL = 3,
    ANIMATION_PHASE_COLUMN_3_INITIAL = 5
H1_ENUM_CONST_END(AdventureAnimationPhaseIndex)

// advManager::ViewWorld's 6-pixel map: cells start 24 pixels in;
// ground6.icn has one frame per four ground tiles, vertically flipped tiles
// 31 frames on and horizontally flipped ones drawn one cell-width minus one
// to the right; tilesets[] is indexed by MapTileset; flag6.icn frames 0..3
// are player colours, 5 marks the current hero and 6 an artifact; town flags
// straddle the cell and resource letters sit 3 pixels left.
H1_ENUM_CONST_BEGIN(ViewWorldConstant)
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
H1_ENUM_CONST_END(ViewWorldConstant)

// The radar panel: 480..624 x 16..160, two pixels per map cell; trees and
// mountains darken the terrain colour by 3 shades and the viewport box is
// drawn in colour 0xbe.
H1_ENUM_CONST_BEGIN(AdventureRadarConstant)
    RADAR_LEFT = 480,
    RADAR_RIGHT = 624,
    RADAR_TOP = 16,
    RADAR_BOTTOM = 160,
    RADAR_SIZE = RADAR_RIGHT - RADAR_LEFT,
    RADAR_CELL_PIXELS = 2,
    RADAR_TERRAIN_SHADE = 3,
    RADAR_VIEWPORT_COLOR = 0xbe
H1_ENUM_CONST_END(AdventureRadarConstant)

// TeleportTo's fizzle (the computed time is not passed on - FizzleForward
// gets the default delay).
H1_ENUM_CONST_BEGIN(AdventureTeleportConstant)
    TELEPORT_FIZZLE_TIME = 128,
    TELEPORT_REMOTE_FIZZLE_ADJUSTMENT = 64
H1_ENUM_CONST_END(AdventureTeleportConstant)

// SummonBoat (the boat's hero flag is game.h BOAT_OCCUPIED_FLAG): the old
// berth is restored with mode 5; the fizzle boxes around the old berth (clamped to
// the viewport's inner box) and at the hero.
H1_ENUM_CONST_BEGIN(AdventureSummonBoatConstant)
    SUMMON_RESTORE_MODE = 5,
    SUMMON_FIZZLE_X_OFFSET = 32,
    SUMMON_FIZZLE_Y_OFFSET = 16,
    SUMMON_FIZZLE_WIDTH = 96,
    SUMMON_FIZZLE_HEIGHT = 48,
    SUMMON_TARGET_X = 176,
    SUMMON_TARGET_Y = 192,
    SUMMON_TARGET_WIDTH = 128,
    SUMMON_TARGET_HEIGHT = 96
H1_ENUM_CONST_END(AdventureSummonBoatConstant)

// ViewPuzzle: puzzle.icn has one piece per obelisk bit
// (playerData::m_puzzlePiecesRemoved); the window sits beside the viewport; the
// view centre is nudged off the artifact by coordinate residues mod 3 (and
// mod 2), then the uncovered pieces fizzle in over 220 ms.
H1_ENUM_CONST_BEGIN(AdventurePuzzleViewConstant)
    PUZZLE_PIECE_COUNT = 48,
    PUZZLE_ALIGNMENT_DIVISOR = 3,
    PUZZLE_Y_ADJUST_X_FACTOR = 2,
    PUZZLE_Y_ADJUST_Y_FACTOR = 5,
    PUZZLE_PARITY_DIVISOR = 2,
    PUZZLE_FIZZLE_TIME = 220
H1_ENUM_CONST_END(AdventurePuzzleViewConstant)

// The network-turn music hold, the walk sample set (played at sample.h
// SAMPLE_VOLUME_FULL), the looping-sample budget per high-memory unit and
// Open's locator scroll knobs.
H1_ENUM_CONST_BEGIN(AdventureStateConstant)
    FORCED_MUSIC_DELAY = 6000,
    CURSOR_SAMPLE_FAST_SET = 2,
    HIGH_MEMORY_BUFFER_DIVISOR = 100,
    // Open's locator scroll knobs (scroll.icn frame 4).
    SCROLL_Y = 195,
    SCROLL_LEFT_X = 540,
    SCROLL_RIGHT_X = 612,
    SCROLL_WIDTH = 8,
    SCROLL_HEIGHT = 17,
    SCROLL_ICON_FRAME = 4
H1_ENUM_CONST_END(AdventureStateConstant)

// SetEnvironmentOrigin/InsertSound's looping map sounds: a slot's ring
// distance (adventureSoundCell::distance, indexing gEnvironmentVolume) resets
// to FAR_DISTANCE, two passes (refresh known sounds, then insert new ones)
// over rings whose edges span radius * 2 cells, and sounds beyond
// MAX_DISTANCE stop.
H1_ENUM_CONST_BEGIN(AdventureEnvironmentSoundConstant)
    ENVIRONMENT_SOUND_FAR_DISTANCE = 127,
    ENVIRONMENT_SOUND_MAX_DISTANCE = 5,
    ENVIRONMENT_SOUND_FIRST_LAYER = 1,
    ENVIRONMENT_SOUND_LAYER_COUNT = 2,
    ENVIRONMENT_SOUND_EDGE_SPAN = 2,
    // SetEnvironmentOrigin's rings around the origin (radius 0..COUNT-1).
    ENVIRONMENT_SOUND_RADIUS_COUNT = 4,
    // InsertSound found no slot to take over.
    ENVIRONMENT_SOUND_NO_SLOT = -1
H1_ENUM_CONST_END(AdventureEnvironmentSoundConstant)

// ComboDraw's dirty-cell grid: cloud-covered neighbours of a moving sprite
// get CLOUD_MARK so the cloud pass redraws them, animation runs every
// FRAME_LIMIT frame steps, the cursor marks two cells right, and the update
// box is clipped to the viewport's inner pixels.
H1_ENUM_CONST_BEGIN(AdventureComboDrawConstant)
    COMBO_CLEAR_BYTES = 256,
    COMBO_CLOUD_MARK = 10,
    COMBO_FRAME_LIMIT = 12,
    COMBO_FAR_NEIGHBOR_OFFSET = 2
H1_ENUM_CONST_END(AdventureComboDrawConstant)

// advManager::Main's debug keys and digit cheat: the typed digits roll into
// a six-digit sequence; 101495 reveals the whole map to every player.
H1_ENUM_CONST_BEGIN(AdventureCheatConstant)
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
H1_ENUM_CONST_END(AdventureCheatConstant)

H1_ENUM_CONST_BEGIN(AdventureEnemyTurnViewConstant)
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
H1_ENUM_CONST_END(AdventureEnemyTurnViewConstant)

// UpdBottomViewNewTurn's date texts (the week line sits at 421).
H1_ENUM_CONST_BEGIN(AdventureNewTurnViewConstant)
    NEW_TURN_DATE_TEXT_X = 479,
    NEW_TURN_WEEK_TEXT_Y = 421,
    NEW_TURN_DAY_TEXT_Y = 438,
    NEW_TURN_DATE_TEXT_WIDTH = 145,
    NEW_TURN_WEEK_TEXT_HEIGHT = 12,
    NEW_TURN_DAY_TEXT_HEIGHT = 25
H1_ENUM_CONST_END(AdventureNewTurnViewConstant)

// UpdBottomViewResMsg's message and resource layout (the text starts at 395
// and the count at 450).
H1_ENUM_CONST_BEGIN(AdventureResourceViewConstant)
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
H1_ENUM_CONST_END(AdventureResourceViewConstant)

// UpdBottomViewKingdom's nine counters: the seven resources (ResourceType
// order), then castles and villages (the text rows start at 392).
H1_ENUM_CONST_BEGIN(AdventureKingdomViewConstant)
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
    KINGDOM_VIEW_WOOD_TEXT_X = 15,
    KINGDOM_VIEW_MERCURY_TEXT_X = 38,
    KINGDOM_VIEW_ORE_TEXT_X = 61,
    KINGDOM_VIEW_SULFUR_TEXT_X = 85,
    KINGDOM_VIEW_CRYSTAL_TEXT_X = 109,
    KINGDOM_VIEW_GEMS_TEXT_X = 132,
    KINGDOM_VIEW_GOLD_TEXT_X = 123,
    KINGDOM_VIEW_CASTLE_TEXT_X = 27,
    KINGDOM_VIEW_VILLAGE_TEXT_X = 80
H1_ENUM_CONST_END(AdventureKingdomViewConstant)

// UpdBottomViewHero's army icons and counts.
H1_ENUM_CONST_BEGIN(AdventureBottomHeroViewConstant)
    BOTTOM_HERO_LABEL_BYTES = 6,
    // Count-label positions and stack counts the label layout distinguishes.
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
H1_ENUM_CONST_END(AdventureBottomHeroViewConstant)

H1_ENUM_CONST_BEGIN(AdventureScrollConstant)
// The adventure view is ADVMGR_VIEW_CELL_COUNT cells square; the hero
// stands on its centre cell, ADVMGR_VIEW_CENTER from the origin.
    ADVMGR_VIEW_CELL_COUNT = 15,
    ADVMGR_VIEW_CENTER = 7,
    SCROLL_MIN_ORIGIN = -ADVMGR_VIEW_CENTER,
    SCROLL_MAX_ORIGIN = MAP_CELL_GRID_SIZE - ADVMGR_VIEW_CENTER - 1,
    SCROLL_TICK_INTERVAL = 70,
    HOVER_SCROLL_FRAME_FIRST = 32,
    HOVER_SCROLL_FRAME_END = 40
H1_ENUM_CONST_END(AdventureScrollConstant)

H1_ENUM_CONST_BEGIN(AdventurePanelDialogConstant)
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
H1_ENUM_CONST_END(AdventurePanelDialogConstant)

// GetCloudLookup's unseen-neighbour bits (index into gCloudType): the four
// edge neighbours, then the diagonals clockwise from north-east; off-map
// columns/rows set their three neighbours at once.
H1_ENUM_FLAGS_BEGIN(CloudNeighborMask, i32)
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
H1_ENUM_FLAGS_END(CloudNeighborMask)

// advManager::Main's right-click help on the six panel buttons: the
// gAdvMenuHelp row (texts: next hero, continue movement, kingdom summary,
// end turn, adventure options, game options).
H1_ENUM_BEGIN(AdventurePanelHelp)
    ADVENTURE_HELP_NONE = -1,
    ADVENTURE_HELP_NEXT_HERO = 0,
    ADVENTURE_HELP_FIRST = ADVENTURE_HELP_NEXT_HERO,
    ADVENTURE_HELP_CONTINUE_ROUTE = 1,
    ADVENTURE_HELP_OVERVIEW = 2,
    ADVENTURE_HELP_END_TURN = 3,
    ADVENTURE_HELP_ADVENTURE_OPTIONS = 4,
    ADVENTURE_HELP_GAME_OPTIONS = 5,
    ADVENTURE_HELP_COUNT = 6
H1_ENUM_END(AdventurePanelHelp)
extern H1_ENUM_ARRAY(char*, gAdvMenuHelp, AdventurePanelHelp, ADVENTURE_HELP_COUNT);

// qhero0/qhero1/qtown1.bin widgets: name, portrait, the hero's four primary
// stats from STAT_FIRST, and the owner's flag pair from FLAG (frames colour
// * 2 and the next). A window x of AT_LOCATOR places the hero view beside
// its locator slot, the town view at its fixed spot.
H1_ENUM_CONST_BEGIN(QuickViewWidget)
    QUICK_VIEW_NAME = 1,
    QUICK_VIEW_PORTRAIT = 2,
    QUICK_VIEW_STAT_FIRST = 3,
    QUICK_VIEW_FLAG = 8,
    QUICK_VIEW_AT_LOCATOR = -1,
    // The map-click views pass no locator slot.
    QUICK_VIEW_NO_LOCATOR = -1
H1_ENUM_CONST_END(QuickViewWidget)

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
    CONTROL_SCENARIO_INFO = 17,
    CONTROL_MUSIC_VOLUME_TEXT = 8,
    CONTROL_SOUND_VOLUME_TEXT = 9,
    CONTROL_WALK_SPEED_TEXT = 10,
    CONTROL_MUSIC_SOURCE_TEXT = 14,
    CONTROL_SHOW_ROUTE_TEXT = 15,
    CONTROL_SHOW_ENEMY_MOVES_TEXT = 16
H1_ENUM_CONST_END(ControlPanelDialogConstant)

// UpdateCPanel's button frames: off/on pairs for music and sound, then one
// frame per walk speed, show-route and enemy-moves state. Music source
// maps its boolean setting to the retained 0/2 display entries.
H1_ENUM_CONST_BEGIN(ControlPanelFrame)
    CPANEL_FRAME_MUSIC_OFF = 10,
    CPANEL_FRAME_MUSIC_ON = 11,
    CPANEL_FRAME_SOUND_OFF = 12,
    CPANEL_FRAME_SOUND_ON = 13,
    CPANEL_FRAME_WALK_SPEED_FIRST = 14,
    CPANEL_FRAME_SHOW_ROUTE_FIRST = 21,
    CPANEL_FRAME_ENEMY_MOVES_FIRST = 23,
    CPANEL_FRAME_MUSIC_SOURCE_FIRST = 27
H1_ENUM_CONST_END(ControlPanelFrame)

// The music-source setting is boolean; the retained labels and frames use
// slots 0 and 2, leaving the original middle quality label unused.
H1_ENUM_CONST_BEGIN(ControlPanelMusicLabel)
    CPANEL_MUSIC_LABEL_LOCAL = 0,
    CPANEL_MUSIC_LABEL_CD = 2
H1_ENUM_CONST_END(ControlPanelMusicLabel)

// CPanelHandler's right-click help: the gCPanelHelp row for each control.
H1_ENUM_BEGIN(ControlPanelHelp)
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
H1_ENUM_END(ControlPanelHelp)
extern H1_ENUM_ARRAY(char*, gCPanelHelp, ControlPanelHelp, CPANEL_HELP_COUNT);

// DimensionDoor's dimdoor.bin dialog: hovering the map view (FIRST_BUTTON)
// sets m_dialogResult to ACCEPT over a free cell, else REJECT, as does the
// other area (LAST_BUTTON); a click with ACCEPT closes the dialog. TownGate
// starts its nearest-town search at DISTANCE_LIMIT.
H1_ENUM_CONST_BEGIN(AdventureTravelSpellConstant)
    TRAVEL_DIALOG_REJECT = 0,
    TRAVEL_DIALOG_ACCEPT = 1,
    DIMENSION_DOOR_FIRST_BUTTON = ADVENTURE_CONTROL_MAP_VIEW,
    DIMENSION_DOOR_LAST_BUTTON = 11,
    TOWN_PORTAL_DISTANCE_LIMIT = 1000
H1_ENUM_CONST_END(AdventureTravelSpellConstant)

// Moved from CURSOR.cpp.
// Hero-cursor drawing and movement constants.
H1_ENUM_CONST_BEGIN(CursorConstant)
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
    // m_cursorCycle: STOPPED, or a walk/turn cycle running (the slow walk
    // starts at SLOW_CURSOR_CYCLE_START).
    CURSOR_CYCLE_STOPPED = 0,
    CURSOR_CYCLE_RUNNING = 1,
    SLOW_CURSOR_CYCLE_START = 2,
    // The gallop skips frames 1 and 4 of the eight-frame walk.
    SKIPPED_ANIMATION_FRAME_EARLY = 1,
    SKIPPED_ANIMATION_FRAME = 4,
    FOOTSTEP_ANIMATION_FRAME = 3,
    DIRECTION_HALF_COUNT = 4,
    TURN_FRAME_MULTIPLIER = 2,
    MOVE_TILE_HALF_COUNT = 2
H1_ENUM_CONST_END(CursorConstant)

#endif // HOMM1_SOURCE_ADVMANAGER_H

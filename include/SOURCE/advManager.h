#ifndef HOMM1_SOURCE_ADVMANAGER_H
#define HOMM1_SOURCE_ADVMANAGER_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 139 methods, 3 own-virtual, 0 static data.

#include <BASE/baseManager.h>
#include <H1/Macros.h>
#include <SOURCE/armySizeNames.h>
#include <SOURCE/artifactTypes.h>
#include <SOURCE/creatureTypes.h>
#include <SOURCE/cursorTypes.h>
#include <SOURCE/dialogTypes.h>
#include <SOURCE/mapCell.h>
#include <SOURCE/resourceTypes.h>
#include <SOURCE/spellTypes.h>

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
struct SAMPLE2;
struct SMapChange;
struct tag_message;

H1_ENUM_CONST_BEGIN(AdventureManagerStorageConstant)
    ADVMGR_BOTTOM_VIEW_WIDGET_COUNT = 12,
    // m_bottomViewPrimaryWidgets slots (Buka 2.1 names): the stone backdrop,
    // the view's foreground icon, then its further icons; the secondary
    // (text) array's army/count labels start at HERO_TEXT_FIRST.
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

// No hero: playerData::m_currentHero/CurrentHero() with nothing selected,
// an empty locator slot, a quick view of nobody (Buka 2.1 ADVMGR
// INVALID_HERO).
H1_ENUM_CONST_BEGIN(AdventureHeroConstant)
    INVALID_HERO = -1
H1_ENUM_CONST_END(AdventureHeroConstant)

// The adventure screen's animation clock: glTimers slot FRAME_TIMER_SLOT
// re-armed TIMER_DELAY ms ahead (advManager::Open/Main/UpdateScreen,
// DimensionDoorHandler, philAI's CheckDoMain).
H1_ENUM_CONST_BEGIN(AdventureFrameTimerConstant)
    ADVENTURE_FRAME_TIMER_SLOT = 0,
    TIMER_DELAY = 120
H1_ENUM_CONST_END(AdventureFrameTimerConstant)

// m_lastQuickViewX/Y: the map cell of the monster DoCombat turns to face the
// attacker (DrawCell draws it facing); the constructor starts it at NONE
// (Buka 2.1 QUICK_VIEW_NONE, -99, off every drawable cell) and DoCombat
// clears the x back to CLEARED (-1) after the redraw.
H1_ENUM_CONST_BEGIN(AdventureQuickViewCellConstant)
    QUICK_VIEW_NONE = -99,
    QUICK_VIEW_CLEARED = -1
H1_ENUM_CONST_END(AdventureQuickViewCellConstant)

// m_lastHoverCell/m_hoverCellY before the mouse hovers a view cell (Buka
// ADVMGR's m_lastHoverCell = CURSOR_INVALID_POSITION).
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

// m_selectedCell: the action ProcessSelect queues and advManager::DoSelect
// runs (Buka 2.1 AdventureCommand, same numbering).
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
// hides on the puzzle map (Buka 2.1 AdventureSearchConstant DIG_HOLE_FRAME,
// HoMM1 frame; Buka passes CURSOR_INVALID_POSITION for the centre).
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
    // span; a click on the track maps the 92-pixel strip from 194 to a page
    // (Buka 2.1 AdventureLocatorConstant names).
    LOCATOR_SCROLL_BASE_Y = 195,
    LOCATOR_HERO_SCROLL_SPAN = 73,
    LOCATOR_TOWN_SCROLL_SPAN = 74,
    LOCATOR_SCROLL_MOUSE_BASE_Y = LOCATOR_SCROLL_BASE_Y - 1,
    LOCATOR_SCROLL_MOUSE_SPAN = 92
H1_ENUM_CONST_END(AdventureLocatorConstant)

// Adventure-window widget ids handled by advManager::Main,
// ProcessSelect/DeSelect/Hover: the six panel buttons, radar, map view and
// the hero/town locator columns (Buka 2.1 ADVMGR.cpp AdventurePanelCommand
// and AdventureLocatorConstant use the same ids; HoMM2 adds buttons 7, 8).
H1_ENUM_BEGIN(AdventureControl)
    ADVENTURE_CONTROL_NEXT_HERO = 1,
    ADVENTURE_CONTROL_CONTINUE_ROUTE = 2,
    ADVENTURE_CONTROL_OVERVIEW = 3,
    ADVENTURE_CONTROL_END_TURN = 4,
    ADVENTURE_CONTROL_ADVENTURE_OPTIONS = 5,
    ADVENTURE_CONTROL_GAME_OPTIONS = 6,
    ADVENTURE_CONTROL_RADAR = 9,
    ADVENTURE_CONTROL_MAP_VIEW = 10,
    ADVENTURE_CONTROL_TOWN_LOCATOR_1 = 16,
    ADVENTURE_CONTROL_TOWN_LOCATOR_2 = 17,
    ADVENTURE_CONTROL_TOWN_LOCATOR_3 = 18,
    ADVENTURE_CONTROL_TOWN_LOCATOR_4 = 19,
    ADVENTURE_CONTROL_HERO_PAGE_PREVIOUS = 20,
    ADVENTURE_CONTROL_HERO_PAGE_NEXT = 21,
    ADVENTURE_CONTROL_HERO_SCROLL = 22,
    ADVENTURE_CONTROL_TOWN_PAGE_PREVIOUS = 23,
    ADVENTURE_CONTROL_TOWN_PAGE_NEXT = 24,
    ADVENTURE_CONTROL_TOWN_SCROLL = 25,
    ADVENTURE_CONTROL_HERO_KNOB = 26,
    ADVENTURE_CONTROL_TOWN_KNOB = 27,
    ADVENTURE_CONTROL_HERO_LOCATOR_1 = 105,
    ADVENTURE_CONTROL_HERO_LOCATOR_2 = 112,
    ADVENTURE_CONTROL_HERO_LOCATOR_3 = 119,
    ADVENTURE_CONTROL_HERO_LOCATOR_4 = 126
H1_ENUM_END(AdventureControl)

// The six panel buttons (ADVENTURE_CONTROL_NEXT_HERO..GAME_OPTIONS) that
// DisableButtons/EnableButtons and game's Show/CancelComputerScreen dim
// (Buka AdventurePanelButtonConstant).
H1_ENUM_CONST_BEGIN(AdventurePanelButtonConstant)
    ADVMGR_PANEL_BUTTON_FIRST = 1,
    ADVMGR_PANEL_BUTTON_LAST = 6
H1_ENUM_CONST_END(AdventurePanelButtonConstant)

struct adventureSoundCell {
    i32 soundId;
    i32 volume;
};

// Retail constructor, Open and InitMainClasses' 0x260-byte allocation fix
// this packed layout after the 0x30-byte baseManager prefix.
#pragma pack(push, 1)
class advManager : public baseManager {
public:
    i8 m_selectedCell;
    class widget* m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_WIDGET_COUNT];
    class widget* m_bottomViewSecondaryWidgets[ADVMGR_BOTTOM_VIEW_WIDGET_COUNT];
    class heroWindow* m_adventureWindow;
    // ShowRoute clears 72*72 bytes and stores signed route frames.
    i8* m_visibilityMap;
    i8 m_routeShown;
    i8 m_currentTerrain;
    char m_unknown9b[4];
    class mapCell (*m_mapData)[MAP_CELL_GRID_SIZE];
    class iconWidget* m_scrollLeftButton;
    class iconWidget* m_scrollRightButton;
    // Open adds these five panel backdrops to the adventure window.
    class backdropWidget* m_panelBackdrops[ADVMGR_PANEL_ICON_COUNT];
    u8* m_adventureBorder;
    char m_unknownc3[4];
    class tileset* m_groundTiles;
    class tileset* m_cloudTiles;
    class tileset* m_stoneTiles;
    class icon* m_objectIcons[ADVMGR_OBJECT_ICON_COUNT];
    class icon* m_puzzleIcon;
    class icon* m_cloudOverlayIcon;
    i16 m_mapOriginX;
    i16 m_mapOriginY;
    i16 m_previousOriginX;
    i16 m_previousOriginY;
    i16 m_lastHoverCell;
    i16 m_hoverCellY;
    i16 m_commandTargetX;
    i16 m_commandTargetY;
    i16 m_updateMinX;
    i16 m_updateMinY;
    i16 m_updateMaxX;
    i16 m_updateMaxY;
    i8 m_animationPhases[ADVMGR_ANIMATION_PHASE_COUNT];
    class icon* m_heroIcons[ADVMGR_HERO_ICON_COUNT];
    class icon* m_boatShadowIcon;
    class icon* m_flagIcons[ADVMGR_PLAYER_COLOR_COUNT];
    class icon* m_boatFlagIcons[ADVMGR_PLAYER_COLOR_COUNT];
    i8 m_cursorActive;
    i8 m_drawHeroShadows;
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
    i8 m_comboHeroDrawn;
    i32 m_heroContextLocked;
    i32 m_townContextLocked;
    i8 m_forceCompleteDraw;
    i8 m_lastQuickViewX;
    i8 m_lastQuickViewY;
    i8 m_mineGuardianFacingLeft;
    i32 m_activeSoundMask;
    adventureSoundCell m_activeSounds[ADVMGR_ACTIVE_SOUND_COUNT];
    class sample* m_loopingSamples[ADVMGR_ENVIRONMENT_SOUND_COUNT];
    class sample* m_cursorSamples[ADVMGR_CURSOR_SAMPLE_COUNT];
    i8 m_identifyHeroActive;
    i8 m_openState;
    // Main drops message types outside this mask (Open sets 0x32f).
    i16 m_messageTypeMask;
    // --- constructors ---
    advManager(void);
    ~advManager();
    // --- virtual methods (vtable order) ---
    virtual i16 Open(i16) OVERRIDE;
    virtual void Close(void) OVERRIDE;
    virtual i16 Main(struct tag_message&) OVERRIDE;
    // --- methods ---
    void StartCursor(i8);
    void StopCursor(i8);
    void DrawCursor(void);
    void DrawCursorShadow(void);
    i16 GetCursorBaseFrame(H1_ENUM_PARAM(MapDirection, i16));
    void TurnTo(i8);
    i32 GetMoveShowIt(i8);
    // HoMM1 retail 0x0040660c: byte direction/flags, seven arguments (ret 0x1c).
    class mapCell* MoveHero(i8, i8, i32*, i32*, i32*, i8, i8*);
    void CheckAdjacentMon(i8*);
    i16 ValidMoveWithEvent(class hero*, i16);
    i16 ValidMove(i16);
    void MoveOrigin(i16, i16);
    void ProcessMapChange(struct SMapChange change);
    void ProcessIncomingSingleMapChange(struct SMapChange* incoming);
    void ProcessIncomingGroupMapChange(char* incomingData);
    void PurgeMapChangeQueue(void);
    void UnwindMapChangeQueue(i32 maximumToUnwind, i32 processChanges);
    void ViewWorld(i8, i8, i8);
    void VWCleanup(void);
    void VWInit(i32 centerX, i32 centerY);
    void VWCompleteDraw(void);
    void GetCursorSampleSet(i32);
    class mapCell* DoAdvCommand(void);
    i32 GetCommandTargetX(void) {
        return m_commandTargetX;
    }
    i32 GetCommandTargetY(void) {
        return m_commandTargetY;
    }
    void CheckSetEvilInterface(i32 redraw, i32 player);
    void Reseed(i32, i32);
    i32 ProcessSelect(struct tag_message*, class mapCell**);
    i32 ProcessDeSelect(struct tag_message*, i32*, class mapCell**);
    i32 ProcessSearch(i32, i32);
    i32 ProcessHover(struct tag_message*);
    void UpdateScreen(i8, i8);
    void CompleteDraw(i16, i16, i32);
    void CompleteDraw(i32);
    i32 GetCloudLookup(i32, i32);
    void DrawCell(i16, i16, i16, i16, i8, i8, i8);
    class mapCell* GetCell(i16, i16);
    void UpdateRadar(i8, i32);
    void QuickInfo(i16, i16);
    void UpdateHeroLocator(i32, i8, i8);
    void UpdateHeroLocators(i8, i8);
    void UpdateTownLocators(i8, i8);
    void UpdBottomView(i8, i8, i8);
    void ClearBottomView(void);
    i8 UpdBottomViewEnemyTurn(void);
    i8 UpdBottomViewNewTurn(void);
    i8 UpdBottomViewResMsg(void);
    i8 UpdBottomViewKingdom(void);
    i8 UpdBottomViewHero(void);
    void HeroQuickView(i8, i8, i16, i16);
    char* GetArmySizeName(i16, H1_ENUM_PARAM(ArmySizeNameVariant, i8));
    void TownQuickView(i8, i8, i16, i16);
    void RedrawAdvScreen(i32);
    void GiveTakeArtifactStat(class hero*, i8, i8);
    void DeactivateCurrTown(void);
    void DeactivateCurrHero(void);
    void MobilizeCurrHero(i32);
    void DemobilizeCurrHero(void);
    void SetTownContext(i8);
    void SetHeroContext(i8, i8);
    void DoHeroKnob(void);
    void DoTownKnob(void);
    void CastSpell(H1_ENUM_PARAM(SpellType, i8));
    void GrabScreen(void);
    void CheckCastSpell(void);
    i8 ComboDraw(i16, i16, i8);
    i8 ComboDraw(i32);
    void SetEnvironmentOrigin(i16, i16, i16);
    void CheckLoadSample(i32);
    i32 GetSoundId(i32 x, i32 y);
    void InsertSound(i16, i16, i16, i8);
    void TeleportTo(i32, i32, i32);
    void DimensionDoor(void);
    void TownGate(void);
    void SummonBoat(void);
    void ShowRoute(i32, i32, i32);
    void HideRoute(i32, i32, i32);
    void CheckDimHero(void);
    void CheckDimNextHeroBut(void);
    void SeedTo(i32, i32);
    void ForceNewHover(void);
    void ScreenScroll(i8, i32);
    void CheckScreenScroll(void);
    i32 MouseInScrollZone(void);
    void SetInitialMapOrigin(void);
    void LoadRemote(void);
    char* CheckHandleNet(void);
    i16 CheckHandleNetPlayerWait(struct tag_message&, i8);
    void TrimLoopingSounds(i32);
    void DisableButtons(void);
    void EnableButtons(void);
    void SaveAdventureBorder(void);
    void DrawAdventureBorder(void);
    i8 FindAdjacentMonster(i32, i32, i32*, i32*, i32, i32);
    void ViewPuzzle(void);
    void PuzzleDraw(i32, i32, i32, i32);
    void AdvPanel(void);
    i16 ControlPanel(void);
    void SystemOptions(void);
    i32 DoVisions(class hero* visionHero);
    i32 IsCrystalBallInEffect(i32 x, i32 y, i32 radius);
    void DoEvent(class mapCell*, i32, i32);
    void EraseObj(class mapCell*, i32, i32);
    void HeroSwap(class hero*, class hero*);
    i32 BarrierEvent(class mapCell*, class hero*);
    void PasswordEvent(class mapCell*, class hero*);
    void GenericSiteEvent(class mapCell* cell, class hero* eventHero);
    void RecruitSiteEvent(class mapCell* cell, class hero* eventHero);
    void ExpansionRecruitEvent(class hero* eventHero, i32 creatureType, i16* availableCount);
    void JailEvent(class mapCell* cell, class hero* eventHero, i32 x, i32 y);
    void TownEvent(class mapCell*, i32, i32);
    void EventSound(i16, i16);
    void EventWindow(
        i16,
        H1_ENUM_PARAM(NormalDialogType, i32),
        char*,
        H1_ENUM_PARAM(NormalDialogResourceType, i32),
        i32,
        H1_ENUM_PARAM(NormalDialogResourceType, i32),
        i32,
        H1_ENUM_PARAM(NormalDialogOrText, i32)
    );
    i32 GiveRandomArtifact(class hero*);
    i32 GiveExperience(class hero*, i32, i8);
    // HoMM1 retail: byte resource, word amount (ret 0xc).
    void GiveResource(class hero*, H1_ENUM_PARAM(ResourceType, i8), i16);
    i16 GiveArtifact(class hero*, H1_ENUM_PARAM(ArtifactType, i8));
    void RecruitEvent(class hero*, H1_ENUM_PARAM(CreatureType, i32), class mapCell*);
    i32 SkeletonEvent(class hero* eventHero, class mapCell* cell, char* text, i32 x, i32 y);
    i32 ZombieEvent(class hero* eventHero, class mapCell* cell, char* text, i32 x, i32 y);
    i8 GhostEvent(class hero*, class mapCell*, i32, i32, i32);
    void HouseEvent(class hero*, class mapCell*);
    // HoMM1 retail: nine arguments (ret 0x24), result in AL.
    i8 CombatMonsterEvent(
        class hero*,
        H1_ENUM_PARAM(CreatureType, i8),
        i16,
        class mapCell*,
        i32,
        i32,
        i8,
        i32,
        i32
    );
    void TransferArtifacts(class hero*, class hero*);
    void HeroLoses(class hero*);
    void DoWhirlpool(class hero*);
    void FizzleCenter(i32);
    void DoAIEvent(class mapCell*, class hero*, i32, i32);
    i32 BarrierAIEvent(class mapCell*, class hero*);
    void PasswordAIEvent(class mapCell*, class hero*);
    void GenericSiteAIEvent(class mapCell* cell, class hero* eventHero);
    void RecruitSiteAIEvent(class mapCell* cell, class hero* eventHero);
    void JailAIEvent(class mapCell* cell, class hero* eventHero, i32 x, i32 y);
    void PlayerMonsterInteract(
        class mapCell*,
        class mapCell*,
        class hero*,
        i8*,
        i32,
        i32,
        i8,
        i32,
        i32
    );
    void ComputerMonsterInteract(class mapCell*, class hero*, i8*);
    i32 DoNetCombat(char*);
    i32 DoCombat(
        i32,
        i32,
        class hero*,
        class armyGroup*,
        class town*,
        class hero*,
        class armyGroup*,
        i32,
        i32,
        i32,
        i8
    );
    void SendHeroTownData(
        i32,
        i32,
        class hero*,
        class armyGroup*,
        class town*,
        class hero*,
        class armyGroup*,
        i32,
        i32,
        i32,
        i8,
        i8,
        i8,
        i8
    );
    void ReceiveHeroTownData(
        char*,
        i32*,
        i32*,
        i32*,
        class hero**,
        class armyGroup**,
        class town**,
        class hero**,
        class armyGroup**,
        i32*,
        i32*,
        i32*,
        i8*,
        i8*,
        i8*
    );
    i32 AutoResolveCombat(
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
        i32 processLosses
    );
};
#pragma pack(pop)

i16 APanelHandler(struct tag_message&);
void UpdateCPanel(i8);
i8 SaveGame(void);
i16 CPanelHandler(struct tag_message&);

// gForceSwitchMusic: the tick a network turn hand-over forced a music
// switch, or IDLE when none is pending (advManager::Main, game::NewDay).
H1_ENUM_CONST_BEGIN(ForcedMusicConstant)
    FORCED_MUSIC_IDLE = -1
H1_ENUM_CONST_END(ForcedMusicConstant)
extern i32 gLastScrollTime;
extern i32 gForceUpdate;
// The adventure screen's bottom-right panel: gCurBottomView is the view
// UpdBottomView last drew, giBottomViewOverride (KB.h) a temporary one that
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
extern i32 gCurBottomView;
extern i32 gCurBottomViewEnemy;
extern i32 iLastAnimFrame;
// UpdBottomViewEnemyTurn's hourglass animation clocks and frames.
extern i32 gSandAnim;
extern i32 gLastHourGlassPhase;
extern i32 gLastHourGlassUpdateTime;
// Volume per environment-sound distance step.
extern const i32 gEnvironmentVolume[];
// gLimitUpdMinX with no pending limit box (UpdateScreen then redraws the
// whole viewport), and m_previousCursorMapX/Y with no hero-cursor cell to
// clear (Buka 2.1 AdventureUpdateScreenConstant UPDATE_NONE).
H1_ENUM_CONST_BEGIN(AdventureUpdateLimitConstant)
    UPDATE_NONE = -1,
    CURSOR_CELL_NONE = -1
H1_ENUM_CONST_END(AdventureUpdateLimitConstant)

extern i32 gLimitUpdMinX;
extern i32 giLimitUpdMinY;
extern i32 giLimitUpdMaxX;
extern i32 giLimitUpdMaxY;
extern class heroWindow* gPanel;
extern i8 bPrefsChanged;
extern i8 gFreshSave;
// ComboDraw's per-view-cell redraw marks and its animation frame clock.
extern i8 bComboDraw[][17];
// DoAdvCommand's route event coordinates handed from MoveHero to DoEvent.
extern i32 TrigX;
extern i32 TrigY;
// CURSOR globals (Buka advManager.h names, CURSOR data): HoMM1 keeps byte
// flags and the last two footstep sample handles (0x0048eb3c/0x0048eb40).
extern i8 gMoveSoundMade;
extern i8 EveryOther;
extern struct _SAMPLE* gPrevMoveSound;
extern struct _SAMPLE* gLastMoveSound;
extern i8 S1cursorDirection;
extern i16 S1cursorBaseFrame;
extern i16 S1cursorFrameCount;
extern i16 S1cursorCycle;
extern i16 S1cursorTurning;
extern i16 gStepDelay[];
// MoveHero's pixels per walk step by speed and the step offsets.
extern i16 gPixelsPerStep[];
extern i16 startVals[];
extern i32 giFrameStep;

struct SMapChange {
    char _pad[64];
};
extern char cArmySizeName[];
extern i32 gCurHourGlassPhase;

#endif // HOMM1_SOURCE_ADVMANAGER_H

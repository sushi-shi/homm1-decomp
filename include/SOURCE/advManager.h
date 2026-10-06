#ifndef HOMM1_SOURCE_ADVMANAGER_H
#define HOMM1_SOURCE_ADVMANAGER_H

#include <BASE/baseManager.h>
#include <SOURCE/armySizeNames.h>
#include <SOURCE/artifactTypes.h>
#include <SOURCE/creatureTypes.h>
#include <SOURCE/cursorTypes.h>
#include <SOURCE/dialogTypes.h>
#include <SOURCE/mapCell.h>
#include <SOURCE/resourceTypes.h>
#include <SOURCE/spellTypes.h>

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

enum AdventureManagerStorageConstant {
    ADVMGR_BOTTOM_VIEW_WIDGET_COUNT = 12,
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
};

enum AdventureHeroConstant {
    INVALID_HERO = -1
};

enum AdventureFrameTimerConstant {
    ADVENTURE_FRAME_TIMER_SLOT = 0,
    TIMER_DELAY = 120
};

enum AdventureQuickViewCellConstant {
    QUICK_VIEW_NONE = -99,
    QUICK_VIEW_CLEARED = -1
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
};

enum AdventurePanelButtonConstant {
    ADVMGR_PANEL_BUTTON_FIRST = 1,
    ADVMGR_PANEL_BUTTON_LAST = 6
};

struct adventureSoundCell {
    i32 soundId;
    i32 volume;
};

#pragma pack(push, 1)
class advManager : public baseManager {
public:
    i8 m_selectedCell;
    class widget* m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_WIDGET_COUNT];
    class widget* m_bottomViewSecondaryWidgets[ADVMGR_BOTTOM_VIEW_WIDGET_COUNT];
    class heroWindow* m_adventureWindow;
    i8* m_visibilityMap;
    i8 m_routeShown;
    i8 m_currentTerrain;
    char m_unknown9b[4];
    class mapCell (*m_mapData)[MAP_CELL_GRID_SIZE];
    class iconWidget* m_scrollLeftButton;
    class iconWidget* m_scrollRightButton;
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
    i16 m_messageTypeMask;
    advManager(void);
    ~advManager();
    virtual i16 Open(i16 id) ;
    virtual void Close(void) ;
    virtual i16 Main(struct tag_message& message) ;
    void StartCursor(i8 direction);
    void StopCursor(i8 stopSound);
    void DrawCursor(void);
    void DrawCursorShadow(void);
    i16 GetCursorBaseFrame(i16 direction);
    void TurnTo(i8 direction);
    i32 GetMoveShowIt(i8 direction);
    class mapCell* MoveHero(
        i8 direction,
        i8 stopAfterMove,
        i32* eventX,
        i32* eventY,
        i32* outOfMobility,
        i8 processEvent,
        i8* adjacentMonster
    );
    void CheckAdjacentMon(i8* adjacentMonster);
    i16 ValidMoveWithEvent(class hero* movingHero, i16 direction);
    i16 ValidMove(i16 direction);
    void MoveOrigin(i16 directionX, i16 directionY);
    void ProcessMapChange(struct SMapChange change);
    void ProcessIncomingSingleMapChange(struct SMapChange* incoming);
    void ProcessIncomingGroupMapChange(char* incomingData);
    void PurgeMapChangeQueue(void);
    void UnwindMapChangeQueue(i32 maximumToUnwind, i32 processChanges);
    void ViewWorld(i8 spellType, i8 drawAllObjects, i8 drawAllTerrains);
    void VWCleanup(void);
    void VWInit(i32 centerX, i32 centerY);
    void VWCompleteDraw(void);
    void GetCursorSampleSet(i32 sampleSet);
    class mapCell* DoAdvCommand(void);
    i32 GetCommandTargetX(void) {
        return m_commandTargetX;
    }
    i32 GetCommandTargetY(void) {
        return m_commandTargetY;
    }
    void CheckSetEvilInterface(i32 redraw, i32 player);
    void Reseed(i32, i32);
    i32 ProcessSelect(struct tag_message* message, class mapCell** eventCell);
    i32 ProcessDeSelect(struct tag_message* message, i32* result, class mapCell** eventCell);
    i32 ProcessSearch(i32 x, i32 y);
    i32 ProcessHover(struct tag_message* message);
    void UpdateScreen(i8 cursorUpdate, i8 forceUpdate);
    void CompleteDraw(i16 originX, i16 originY, i32 forceDraw);
    void CompleteDraw(i32 update);
    i32 GetCloudLookup(i32 x, i32 y);
    void DrawCell(
        i16 mapX,
        i16 mapY,
        i16 screenX,
        i16 screenY,
        i8 drawMask,
        i8 drawingPuzzle,
        i8 forceDraw
    );
    class mapCell* GetCell(i16 x, i16 y);
    void UpdateRadar(i8 updateScreen, i32 partial);
    void QuickInfo(i16 cellX, i16 cellY);
    void UpdateHeroLocator(i32 locatorSlot, i8 drawWindow, i8 updateScreen);
    void UpdateHeroLocators(i8 drawWindow, i8 updateScreen);
    void UpdateTownLocators(i8 drawWindow, i8 updateScreen);
    void UpdBottomView(i8 forceUpdate, i8 drawWindow, i8 updateScreen);
    void ClearBottomView(void);
    i8 UpdBottomViewEnemyTurn(void);
    i8 UpdBottomViewNewTurn(void);
    i8 UpdBottomViewResMsg(void);
    i8 UpdBottomViewKingdom(void);
    i8 UpdBottomViewHero(void);
    void HeroQuickView(i8 heroId, i8 locatorSlot, i16 windowX, i16 windowY);
    char* GetArmySizeName(i16 armySize, i8 grammar);
    void TownQuickView(i8 townId, i8, i16 windowX, i16 windowY);
    void RedrawAdvScreen(i32 update);
    void GiveTakeArtifactStat(class hero* targetHero, i8 artifact, i8 take);
    void DeactivateCurrTown(void);
    void DeactivateCurrHero(void);
    void MobilizeCurrHero(i32 update);
    void DemobilizeCurrHero(void);
    void SetTownContext(i8 townId);
    void SetHeroContext(i8 heroId, i8 update);
    void DoHeroKnob(void);
    void DoTownKnob(void);
    void CastSpell(i8 spell);
    void GrabScreen(void);
    void CheckCastSpell(void);
    i8 ComboDraw(i16 originX, i16 originY, i8 animate);
    i8 ComboDraw(i32 update);
    void SetEnvironmentOrigin(i16 originX, i16 originY, i16 stopSounds);
    void CheckLoadSample(i32 index);
    i32 GetSoundId(i32 x, i32 y);
    void InsertSound(i16 x, i16 y, i16 distance, i8 soundLayer);
    void TeleportTo(i32 x, i32 y, i32);
    void DimensionDoor(void);
    void TownGate(void);
    void SummonBoat(void);
    void ShowRoute(i32 redraw, i32, i32 updateButton);
    void HideRoute(i32 redraw, i32 clearDestination, i32 updateButton);
    void CheckDimHero(void);
    void CheckDimNextHeroBut(void);
    void SeedTo(i32 targetX, i32 targetY);
    void ForceNewHover(void);
    void ScreenScroll(i8 direction, i32 updatePointer);
    void CheckScreenScroll(void);
    i32 MouseInScrollZone(void);
    void SetInitialMapOrigin(void);
    void LoadRemote(void);
    char* CheckHandleNet(void);
    i16 CheckHandleNetPlayerWait(struct tag_message& message, i8 doMain);
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
    void SystemOptions(void);
    i32 DoVisions(class hero* visionHero);
    i32 IsCrystalBallInEffect(i32 x, i32 y, i32 radius);
    void DoEvent(class mapCell* cell, i32 x, i32 y);
    void EraseObj(class mapCell* cell, i32 x, i32 y);
    void HeroSwap(class hero* firstHero, class hero* secondHero);
    i32 BarrierEvent(class mapCell*, class hero*);
    void PasswordEvent(class mapCell*, class hero*);
    void GenericSiteEvent(class mapCell* cell, class hero* eventHero);
    void RecruitSiteEvent(class mapCell* cell, class hero* eventHero);
    void ExpansionRecruitEvent(class hero* eventHero, i32 creatureType, i16* availableCount);
    void JailEvent(class mapCell* cell, class hero* eventHero, i32 x, i32 y);
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
    i32 GiveRandomArtifact(class hero* eventHero);
    i32 GiveExperience(class hero* eventHero, i32 experience, i8 checkLevel);
    void GiveResource(class hero* eventHero, i8 resource, i16 amount);
    i16 GiveArtifact(class hero* eventHero, i8 artifact);
    void RecruitEvent(
        class hero* eventHero,
        i32 creatureType,
        class mapCell* cell
    );
    i32 SkeletonEvent(class hero* eventHero, class mapCell* cell, char* text, i32 x, i32 y);
    i32 ZombieEvent(class hero* eventHero, class mapCell* cell, char* text, i32 x, i32 y);
    i8 GhostEvent(class hero* eventHero, class mapCell* cell, i32 textId, i32 x, i32 y);
    void HouseEvent(class hero* eventHero, class mapCell* cell);
    i8 CombatMonsterEvent(
        class hero* eventHero,
        i8 monsterType,
        i16 count,
        class mapCell* cell,
        i32 x,
        i32 y,
        i8 heroDefends,
        i32 fromX,
        i32 fromY
    );
    void TransferArtifacts(class hero* sourceHero, class hero* destHero);
    void HeroLoses(class hero* lostHero);
    void DoWhirlpool(class hero* eventHero);
    void FizzleCenter(i32 fizzleType);
    void DoAIEvent(class mapCell* cell, class hero* eventHero, i32 x, i32 y);
    i32 BarrierAIEvent(class mapCell*, class hero*);
    void PasswordAIEvent(class mapCell*, class hero*);
    void GenericSiteAIEvent(class mapCell* cell, class hero* eventHero);
    void RecruitSiteAIEvent(class mapCell* cell, class hero* eventHero);
    void JailAIEvent(class mapCell* cell, class hero* eventHero, i32 x, i32 y);
    void PlayerMonsterInteract(
        class mapCell* cell,
        class mapCell* combatCell,
        class hero* eventHero,
        i8* handled,
        i32 x,
        i32 y,
        i8 unused,
        i32 combatX,
        i32 combatY
    );
    void ComputerMonsterInteract(class mapCell* cell, class hero* eventHero, i8* handled);
    i32 DoNetCombat(char* packet);
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
        i8 processLosses
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
        char* packet,
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

i16 APanelHandler(struct tag_message& message);
void UpdateCPanel(i8 initialDraw);
i8 SaveGame(void);
i16 CPanelHandler(struct tag_message& message);

enum ForcedMusicConstant {
    FORCED_MUSIC_IDLE = -1
};
extern i32 gLastScrollTime;
extern i32 gForceUpdate;
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
extern i32 gCurBottomViewEnemy;
extern i32 iLastAnimFrame;
extern i32 gSandAnim;
extern i32 gLastHourGlassPhase;
extern i32 gLastHourGlassUpdateTime;
extern const i32 gEnvironmentVolume[];
enum AdventureUpdateLimitConstant {
    UPDATE_NONE = -1,
    CURSOR_CELL_NONE = -1
};

extern i32 gLimitUpdMinX;
extern i32 giLimitUpdMinY;
extern i32 giLimitUpdMaxX;
extern i32 giLimitUpdMaxY;
extern class heroWindow* gPanel;
extern i8 bPrefsChanged;
extern i8 gFreshSave;
extern i8 bComboDraw[][17];
extern i32 TrigX;
extern i32 TrigY;
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
extern i16 gPixelsPerStep[];
extern i16 startVals[];
extern i32 giFrameStep;

struct SMapChange {
    char _pad[64];
};
extern char cArmySizeName[];
extern i32 gCurHourGlassPhase;

#endif

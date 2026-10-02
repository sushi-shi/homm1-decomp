#ifndef HOMM1_SOURCE_ADVMANAGER_H
#define HOMM1_SOURCE_ADVMANAGER_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 139 methods, 3 own-virtual, 0 static data.

#include <BASE/baseManager.h>
#include <H1/Macros.h>
#include <SOURCE/armySizeNames.h>
#include <SOURCE/cursorTypes.h>
#include <SOURCE/mapCell.h>

// forward declarations:
class armyGroup;
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

// clang-format off
H1_ENUM_BEGIN(AdventureManagerStorageConstant)
    ADVMGR_BOTTOM_VIEW_WIDGET_COUNT = 12,
    ADVMGR_OBJECT_ICON_COUNT = 21,
    ADVMGR_PANEL_ICON_COUNT = 5,
    ADVMGR_ANIMATION_PHASE_COUNT = 4,
    ADVMGR_HERO_ICON_COUNT = 5,
    ADVMGR_PLAYER_COLOR_COUNT = 4,
    ADVMGR_ACTIVE_SOUND_COUNT = 8,
    ADVMGR_ENVIRONMENT_SOUND_COUNT = 22,
    ADVMGR_CURSOR_SAMPLE_COUNT = 7
H1_ENUM_END(AdventureManagerStorageConstant)
 // clang-format on

 struct adventureSoundCell {
    int soundId;
    int volume;
};

// Retail constructor, Open and InitMainClasses' 0x260-byte allocation fix
// this packed layout after the 0x30-byte baseManager prefix.
#pragma pack(push, 1)
class advManager : public baseManager {
public:
    signed char m_selectedCell;
    class widget* m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_WIDGET_COUNT];
    class widget* m_bottomViewSecondaryWidgets[ADVMGR_BOTTOM_VIEW_WIDGET_COUNT];
    class heroWindow* m_adventureWindow;
    // ShowRoute clears 72*72 bytes and stores signed route frames.
    signed char* m_visibilityMap;
    signed char m_routeShown;
    signed char m_currentTerrain;
    char m_unknown9b[4];
    class mapCell (*m_mapData)[MAP_CELL_GRID_SIZE];
    class iconWidget* m_scrollLeftButton;
    class iconWidget* m_scrollRightButton;
    class icon* m_panelIcons[ADVMGR_PANEL_ICON_COUNT];
    unsigned char* m_adventureBorder;
    char m_unknownc3[4];
    class tileset* m_groundTiles;
    class tileset* m_cloudTiles;
    class tileset* m_stoneTiles;
    class icon* m_objectIcons[ADVMGR_OBJECT_ICON_COUNT];
    class icon* m_puzzleIcon;
    class icon* m_cloudOverlayIcon;
    short m_mapOriginX;
    short m_mapOriginY;
    short m_previousOriginX;
    short m_previousOriginY;
    short m_lastHoverCell;
    short m_hoverCellY;
    short m_commandTargetX;
    short m_commandTargetY;
    short m_updateMinX;
    short m_updateMinY;
    short m_updateMaxX;
    short m_updateMaxY;
    signed char m_animationPhases[ADVMGR_ANIMATION_PHASE_COUNT];
    class icon* m_heroIcons[ADVMGR_HERO_ICON_COUNT];
    class icon* m_boatShadowIcon;
    class icon* m_flagIcons[ADVMGR_PLAYER_COLOR_COUNT];
    class icon* m_boatFlagIcons[ADVMGR_PLAYER_COLOR_COUNT];
    signed char m_cursorActive;
    signed char m_drawHeroShadows;
    unsigned char m_cursorType;
    signed char m_cursorDirection;
    short m_cursorFrame;
    short m_cursorFrameCount;
    short m_cursorCycle;
    short m_cursorTurning;
    short m_cursorMapX;
    short m_previousCursorMapX;
    short m_cursorMapY;
    short m_previousCursorMapY;
    signed char m_comboHeroDrawn;
    int m_heroContextLocked;
    int m_townContextLocked;
    signed char m_forceCompleteDraw;
    signed char m_lastQuickViewX;
    signed char m_lastQuickViewY;
    signed char m_mineGuardianFacingLeft;
    int m_activeSoundMask;
    adventureSoundCell m_activeSounds[ADVMGR_ACTIVE_SOUND_COUNT];
    class sample* m_loopingSamples[ADVMGR_ENVIRONMENT_SOUND_COUNT];
    class sample* m_cursorSamples[ADVMGR_CURSOR_SAMPLE_COUNT];
    signed char m_identifyHeroActive;
    signed char m_openState;
    short m_unknown25e;
    // --- constructors ---
    advManager(void);
    ~advManager();
    // --- virtual methods (vtable order) ---
    virtual short Open(short) OVERRIDE;
    virtual void Close(void) OVERRIDE;
    virtual short Main(struct tag_message&) OVERRIDE;
    // --- methods ---
    void StartCursor(signed char);
    void StopCursor(signed char);
    void DrawCursor(void);
    void DrawCursorShadow(void);
    short GetCursorBaseFrame(H1_ENUM_PARAM(MapDirection, short));
    void TurnTo(signed char);
    int GetMoveShowIt(signed char);
    // Retail ret 0x1c: byte direction/stop flags and a byte interrupt out-flag.
    class mapCell* MoveHero(signed char, signed char, int*, int*, int*, signed char, signed char*);
    void CheckAdjacentMon(signed char*);
    short ValidMoveWithEvent(class hero*, short);
    short ValidMove(short);
    void MoveOrigin(short, short);
    void ProcessMapChange(struct SMapChange);
    void ProcessIncomingSingleMapChange(struct SMapChange*);
    void ProcessIncomingGroupMapChange(char*);
    void PurgeMapChangeQueue(void);
    void UnwindMapChangeQueue(int, int);
    void ViewWorld(signed char, signed char, signed char);
    void VWCleanup(void);
    void VWInit(int, int);
    void VWCompleteDraw(void);
    void GetCursorSampleSet(int);
    class mapCell* DoAdvCommand(void);
    void CheckSetEvilInterface(int, int);
    void Reseed(int, int);
    int ProcessSelect(struct tag_message*, class mapCell**);
    int ProcessDeSelect(struct tag_message*, int*, class mapCell**);
    int ProcessSearch(int, int);
    int ProcessHover(struct tag_message*);
    void UpdateScreen(signed char, signed char);
    void CompleteDraw(short, short, int);
    void CompleteDraw(int);
    int GetCloudLookup(int, int);
    void DrawCell(short, short, short, short, signed char, signed char, signed char);
    class mapCell* GetCell(short, short);
    void UpdateRadar(signed char, int);
    void QuickInfo(short, short);
    void UpdateHeroLocator(int, signed char, signed char);
    void UpdateHeroLocators(signed char, signed char);
    void UpdateTownLocators(signed char, signed char);
    void UpdBottomView(signed char, signed char, signed char);
    void ClearBottomView(void);
    signed char UpdBottomViewEnemyTurn(void);
    signed char UpdBottomViewNewTurn(void);
    signed char UpdBottomViewResMsg(void);
    signed char UpdBottomViewKingdom(void);
    signed char UpdBottomViewHero(void);
    void HeroQuickView(signed char, signed char, short, short);
    char* GetArmySizeName(short, H1_ENUM_PARAM(ArmySizeNameVariant, signed char));
    void TownQuickView(signed char, signed char, short, short);
    void RedrawAdvScreen(int);
    void GiveTakeArtifactStat(class hero*, signed char, signed char);
    void DeactivateCurrTown(void);
    void DeactivateCurrHero(void);
    void MobilizeCurrHero(int);
    void DemobilizeCurrHero(void);
    void SetTownContext(signed char);
    void SetHeroContext(signed char, signed char);
    void DoHeroKnob(void);
    void DoTownKnob(void);
    void CastSpell(signed char);
    void GrabScreen(void);
    void CheckCastSpell(void);
    signed char ComboDraw(short, short, signed char);
    signed char ComboDraw(int);
    void SetEnvironmentOrigin(short, short, short);
    void CheckLoadSample(int);
    int GetSoundId(int, int);
    void InsertSound(short, short, short, signed char);
    void TeleportTo(int, int, int);
    void DimensionDoor(void);
    void TownGate(void);
    void SummonBoat(void);
    void ShowRoute(int, int, int);
    void HideRoute(int, int, int);
    void CheckDimHero(void);
    void CheckDimNextHeroBut(void);
    void SeedTo(int, int);
    void ForceNewHover(void);
    void ScreenScroll(signed char, int);
    void CheckScreenScroll(void);
    int MouseInScrollZone(void);
    void SetInitialMapOrigin(void);
    void LoadRemote(void);
    char* CheckHandleNet(void);
    short CheckHandleNetPlayerWait(struct tag_message &, signed char);
    void TrimLoopingSounds(int);
    void DisableButtons(void);
    void EnableButtons(void);
    void SaveAdventureBorder(void);
    void DrawAdventureBorder(void);
    signed char FindAdjacentMonster(int, int, int*, int*, int, int);
    void ViewPuzzle(void);
    void PuzzleDraw(int, int, int, int);
    void AdvPanel(void);
    short ControlPanel(void);
    void SystemOptions(void);
    int DoVisions(class hero*);
    int IsCrystalBallInEffect(int, int, int);
    void DoEvent(class mapCell*, int, int);
    void EraseObj(class mapCell*, int, int);
    void HeroSwap(class hero*, class hero*);
    int BarrierEvent(class mapCell*, class hero*);
    void PasswordEvent(class mapCell*, class hero*);
    void GenericSiteEvent(class mapCell*, class hero*);
    void RecruitSiteEvent(class mapCell*, class hero*);
    void ExpansionRecruitEvent(class hero*, int, short int*);
    void JailEvent(class mapCell*, class hero*, int, int);
    void TownEvent(class mapCell*, int, int);
    void EventSound(short, short);
    void EventWindow(short, int, char*, int, int, int, int, int);
    int GiveRandomArtifact(class hero*);
    int GiveExperience(class hero*, int, signed char);
    // HoMM1 retail: byte resource, word amount (ret 0xc).
    void GiveResource(class hero*, signed char, short);
    short GiveArtifact(class hero*, signed char);
    void RecruitEvent(class hero*, int, class mapCell*);
    int SkeletonEvent(class hero*, class mapCell*, char*, int, int);
    int ZombieEvent(class hero*, class mapCell*, char*, int, int);
    signed char GhostEvent(class hero*, class mapCell*, int, int, int);
    void HouseEvent(class hero*, class mapCell*);
    // HoMM1 retail: nine arguments (ret 0x24), result in AL.
    signed char CombatMonsterEvent(class hero*, signed char, short, class mapCell*, int, int,
                                   signed char, int, int);
    void TransferArtifacts(class hero*, class hero*);
    void HeroLoses(class hero*);
    void DoWhirlpool(class hero*);
    void FizzleCenter(int);
    void DoAIEvent(class mapCell*, class hero*, int, int);
    int BarrierAIEvent(class mapCell*, class hero*);
    void PasswordAIEvent(class mapCell*, class hero*);
    void GenericSiteAIEvent(class mapCell*, class hero*);
    void RecruitSiteAIEvent(class mapCell*, class hero*);
    void JailAIEvent(class mapCell*, class hero*, int, int);
    void PlayerMonsterInteract(
        class mapCell*,
        class mapCell*,
        class hero*,
        signed char*,
        int,
        int,
        int,
        int,
        int
    );
    void ComputerMonsterInteract(class mapCell*, class hero*, signed char*);
    int DoNetCombat(char*);
    int DoCombat(
        int,
        int,
        class hero*,
        class armyGroup*,
        class town*,
        class hero*,
        class armyGroup*,
        int,
        int,
        int,
        int
    );
    void SendHeroTownData(
        int,
        int,
        class hero*,
        class armyGroup*,
        class town*,
        class hero*,
        class armyGroup*,
        int,
        int,
        int,
        int,
        int,
        int,
        int
    );
    void ReceiveHeroTownData(
        char*,
        int*,
        int*,
        int*,
        class hero**,
        class armyGroup**,
        class town**,
        class hero**,
        class armyGroup**,
        int*,
        int*,
        int*,
        signed char*,
        signed char*,
        signed char*
    );
    int AutoResolveCombat(
        int,
        int,
        class hero*,
        class armyGroup*,
        class town*,
        class hero*,
        class armyGroup*,
        int,
        int,
        int,
        int
    );
};
#pragma pack(pop)

short APanelHandler(struct tag_message &);
void UpdateCPanel(signed char);
int SaveGame(void);
short CPanelHandler(struct tag_message &);

extern int gbNoBorder;
extern int gbRemoteOn;
extern long giForceSwitchMusic;
extern long iLastScrollTime;
extern int gbForceUpdate;
extern int gbAllBlack;
extern int giFullySeeded;
extern class searchArray *gpSearchArray;
extern int iCurBottomView;
extern int iCurBottomViewEnemy;
extern int iLastAnimFrame;
// UpdBottomViewEnemyTurn's hourglass animation clocks and frames.
extern long iLastSandAnimTime;
extern long iLastNewSandAnimTime;
extern int iSandAnim;
extern int iLastHourGlassPhase;
extern long giLastHourGlassUpdateTime;
extern signed char giShowComputerRoute;
extern short gMapX;
extern short gMapY;
extern unsigned char giCurWatchPlayerBit;
extern short gGameCommand;
extern int gbHeroMoving;
extern unsigned char giCurPlayerBit;
// Volume per environment-sound distance step.
extern long glEnvironmentVolume[];
// Route arrow frame by [next step][this step] path direction.
extern signed char gRouteFrame[][8];
// Per hero type scouting radius used by TeleportTo.
extern signed char gHeroScoutRadius[];
extern int giLimitUpdMinX;
extern int giLimitUpdMinY;
extern int giLimitUpdMaxX;
extern int giLimitUpdMaxY;
extern class heroWindow *cPanel;
extern signed char bPrefsChanged;
extern signed char bFreshSave;
extern unsigned char giCloudType[];
// UpdBottomViewHero's per-creature mons32.icn frame width.
extern signed char gMons32Width[];
// ComboDraw's per-view-cell redraw marks and its animation frame clock.
extern signed char bComboDraw[][17];
extern int giFrameCount;
// UpdateRadar's per-owner and per-terrain radar pixel colours.
extern short gRadarOwnerColor[];
extern short gRadarTerrainColor[];
void ComputeUALoc(int);
#endif // HOMM1_SOURCE_ADVMANAGER_H

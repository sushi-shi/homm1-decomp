#ifndef HOMM1_SOURCE_COMBATMANAGER_H
#define HOMM1_SOURCE_COMBATMANAGER_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 149 methods, 3 own-virtual, 0 static data.

#include <BASE/baseManager.h>
#include <Domains.h>
#include <H1/Macros.h>
#include <SOURCE/army.h>
#include <SOURCE/hexcell.h>

// forward declarations:
class army;
class armyGroup;
class hero;
class heroWindow;
class icon;
class town;
struct SBolt;
struct tag_message;

// Buka's combat command/pointer domain, narrowed to the two values used by
// HoMM1 GetPointer; retail compares command 13 and returns pointer 5.
H1_ENUM_BEGIN(CombatPointerCode)
    COMBAT_MESSAGE_COMMAND_OPPOSING_OPTIONS = 13,
    COMBAT_POINTER_VIEW = 5
H1_ENUM_END(CombatPointerCode)

// clang-format off
H1_ENUM_BEGIN(CombatGridConstant)
    COMBAT_HEX_COUNT = 45,
    COMBAT_SIDE_ARMY_COUNT = 6
H1_ENUM_END(CombatGridConstant)
// clang-format on

// Buka CombatRemotePacket: the combat action relayed through
// GetRemoteData (command 0x17) or a net chat line (command 0xb).
#pragma pack(push, 1)
struct CombatRemotePacket {
    signed char sender;
    int id;
    signed char type;
    signed char command;
    short payloadSize;
    union {
        struct {
            int nextAction;
            int nextActionExtra;
            int nextActionGridIndex;
            int nextActionGridIndex2;
        };
        char text[0xf7];
    };
};
#pragma pack(pop)

// HoMM1 combat manager, 0x7d3 bytes (InitMainClasses; constructor
// 0x0044b440). Field names follow Buka where the retail use matches;
// unrecovered spans stay opaque. GameUnsaved reads the baseManager m_active
// word through gpCombatManager.
#pragma pack(push, 1)
class combatManager : public baseManager {
public:
    char m_unknown30[0x10];
    hexcell m_hexCells[COMBAT_HEX_COUNT];
    short m_unknown25c;
    // DrawFrame skips the grid overlay while this is set.
    signed char m_gridMode;
    char m_unknown25f;
    signed char m_unknown260;
    char m_unknown261[4];
    // army::DrawToBuffer prints stack quantities with this font.
    class font* m_smallFont;
    char m_unknown269[8];
    // Nine combat icons (retail loops 0..8 from +0x271): hexcell draws
    // ground (index), obstacles (2), towers (5) and walls (6); armies draw
    // the quantity box (1) and spell markers (8).
    class icon* m_combatIcons[9];
    // Clean combat background; army::Walk restores the screen from it.
    class bitmap* m_backgroundBitmap;
    signed char m_unknown299;
    char m_unknown29a[4];
    // DoVictory: an attacker winning here earns the castle bonus.
    class town *m_combatTown;
    char m_unknown2a2[4];
    class hero *m_heroes[2];
    char m_unknown2ae[8];
    // Set by a surrender (ProcessNextAction).
    signed char m_sideDefeated[2];
    char m_unknown2b8[2];
    signed char m_playerId[2];
    int m_experienceValue[2];
    signed char m_heroCastSpell[2];
    // Stacks per side; the AI mask helpers loop over these.
    short m_numArmies[2];
    army m_armies[2][COMBAT_SIDE_ARMY_COUNT];
    signed char m_currentSide;
    signed char m_currentArmyIndex;
    signed char m_currentSpeed;
    signed char m_gridSelectionDisabled;
    signed char m_limitCreature;
    signed char m_limitCreatureHex;
    signed char m_unknown6c0;
    signed char m_selectedHex;
    signed char m_directionTargetHex;
    signed char m_previousCommand;
    signed char m_currentCommand;
    short m_unknown6c5;
    short m_unknown6c7;
    short m_catapultAttackCount[2];
    short m_catapultAttacksRemaining[2];
    short m_keepAttacksRemaining[2];
    short m_unknown6d5;
    short m_unknown6d7;
    short m_unknown6d9;
    short m_unknown6db;
    // Per side: the side fights from a castle. hexcell::DrawTower/DrawWall
    // mirror the castle art from side 1's flag.
    signed char m_castleSide[2];
    char m_unknown6df[4];
    signed char m_unknown6e3;
    short m_wallFrame;
    short m_wallDamage;
    char m_unknown6e8[0x13];
    // ProcessCombatMsg ignores message types outside this mask.
    short m_messageTypeMask;
    signed char m_sideRetreated[2];
    // Per stack draw state: ResetLimitCreature clears it (-1 for the dead)
    // and army::SpellEffect marks the stack it animates.
    int m_limitCreatureCount[2][5];
    // Set before a full combat redraw.
    int m_unknown727;
    int m_unknown72b;
    char m_unknown72f[4];
    class widget *m_winLoseBottomWidgets[15];
    class widget *m_winLoseBottomTextWidgets[15];
    char m_unknown7ab[8];
    // SetCombatDirections: attack direction per 15-degree mouse sector.
    signed char m_directionMap[24];
    signed char m_mouseDirection;
    signed char m_validDirectionCount;
    class heroWindow *m_winLoseWindow;
    signed char m_selectedSpell;
    // advManager::DoCombat returns and hands on this outcome byte.
    signed char m_combatResult;
    // --- constructors ---
    combatManager(void);
    // --- virtual methods (vtable order) ---
    virtual short Open(short) OVERRIDE;
    virtual void Close(void) OVERRIDE;
    virtual short Main(struct tag_message &) OVERRIDE;
    // --- methods ---
    void NoShowCombatLog(char *);
    void ClearCombatMessages(int);
    void CheckUpdateCombatMessages(void);
    // HoMM1 retail 0x00470aa9: text and a redraw flag (ret 8).
    void CombatMessage(char *, int);
    // HoMM1 retail 0x00470b5e: command help line (ret 4).
    void CombatMessage(short);
    void ResetLimitCreature(void);
    void UpdateCombatArea(void);
    void SetupGridForArmy(class army *);
    // HoMM1 retail 0x004709f0: word first hex, redraw flag (ret 8).
    void UpdateGrid(short, int);
    void DrawBackground(void);
    void UpdateMouseGrid(int, int);
    // HoMM1 retail 0x004711fb takes only the update flag (ret 4).
    void DrawFrame(int);
    // HoMM1 retail 0x00470f25: byte mode (ret 4).
    void SetGridMode(signed char);
    void DrawSmallView(int, int);
    int ViewGeneral(int, int, int);
    // HoMM1 retail 0x00438a9f: army, side and a quick-view flag (ret 0xc).
    void ViewArmy(class army *, int, int);
    int HasValidSpellTarget(int);
    signed char ViewSpells(int);
    int FindResurrectArmyIndex(int, int, int);
    int ValidSpellTarget(int, int);
    void SpellMessage(int, int);
    void CastSpell(int, int, int, int);
    void DefaultSpell(int);
    void Fireball(int, int);
    void MeteorShower(int);
    void ElementalStorm(void);
    void Armageddon(void);
    void TurnToStone(class army *);
    void BloodLustEffect(class army *, int);
    void Ripple(int);
    void Blur(int, int, int);
    void ResetBoltAngle(struct SBolt *);
    void DrawBolt(struct SBolt *, int);
    void AddBolt(struct SBolt *, int, int, int, int, int, int, int, int, int, int, int, int);
    void DoBolt(int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int);
    int GetNextChainLightningTarget(class army *, int);
    void ChainLightning(int, int);
    void VaporizeCreature(int, int);
    void RippleCreature(int, int, int);
    void ShowMassSpell(signed char (* const)[20], int, int);
    void CastMassSpell(int, int);
    void MirrorImage(int);
    void SummonElemental(int, int);
    void DoLuck(int, int);
    void DoBlast(int, int);
    void Resurrect(int, int, int);
    int SpaceForElementalExists(void);
    void ShowSpellCastFailure(class army *, int);
    void ModifyDamageForArtifacts(long int *, int, class hero *, class hero *);
    void Earthquake(void);
    void ShowSpellMessage(int, int, class army *);
    signed char ValidHexToStandOn(int);
    void SetCombatDirections(int);
    void CheckSetMouseDirection(int, int, int);
    H1_ENUM_RETURN(CombatPointerCode, int) GetPointer(H1_ENUM_PARAM(CombatPointerCode, int));
    int ProcessCombatMsg(struct tag_message &);
    int IsNegationSphereInEffect(void);
    void ResetRound(void);
    int CheckWin(struct tag_message *);
    signed char GetCommand(short);
    signed char RightClick(signed char);
    void DoCommand(signed char);
    void ClearWinLoseBottom(class heroWindow *);
    void ShowWinLoseArtifact(class heroWindow *, int);
    void ShowSkeletons(class heroWindow *);
    void ShowEagleEyeSpell(class heroWindow *);
    void ShowDeadArmies(class heroWindow *);
    void DoVictory(signed char);
    void DoLoseWindow(void);
    short DoSurrender(void);
    void CheckChangeSelector(void);
    void CheckCastleAttack(void);
    void CheckGetAIMove(void);
    void GetControl(void);
    void ResetMouse(void);
    short ProcessNextAction(struct tag_message &);
    void ResetCyclingCreatures(void);
    void ResetCycleTimers(void);
    void CycleCombatScreen(void);
    void SetCombatViewArmySmallLevel(int);
    void SetCombatGrid(int, int, int);
    void AddArmy(int, int, int, int, int, int);
    void SetupSmallView(void);
    void ViewBallista(int);
    // HoMM1 retail 0x00437010: byte side (ret 4).
    int DoSpellAI(signed char);
    void DetermineEffectOfSpell(int, int *, int *);
    int EffectSpellCreateCreature(int, int);
    int RawEffectSpellInfluence(class army *, int);
    void ClearEffects(void);
    void NextPos(int *);
    int FirstArmy(int, int, int *);
    int FirstResurrectable(int, int *, int);
    void EffectSpellCure(int *, int, int, int);
    void EffectSpellResurrect(int *, int, int);
    void EffectSpellDamage(int *, int, int);
    void CombineGroups(class armyGroup *, class armyGroup *);
    void SetupCombat(int, int, class hero *, class armyGroup *, class town *, class hero *, class armyGroup *, int, int, int);
    void InitNonVisualVars(void);
    void SetupAdjacencyArray(void);
    void UpdateArmyGroup(int);
    void GenerateMap(void);
    char * GetBackgroundName(void);
    int MoreTreesNear(void);
    void LoadIcons(void);
    void FreeIcons(void);
    void LoadArmies(void);
    void FreeArmies(void);
    short GetGridIndex(short, short);
    void CheckApplyGoodMorale(int, int);
    int CheckApplyBadMorale(int, int);
    // HoMM1 returns the found flag in AL.
    signed char GetNextArmy(int);
    // HoMM1 retail 0x0044d9ca: byte side, byte result.
    signed char IsWinner(signed char);
    void CatAttack(signed char);
    // HoMM1 has a single keep (retail 0x0044e840, plain ret).
    void KeepAttack(void);
    int ExperienceValueOfStack(int);
    void ResetHitByCreature(void);
    void SaveCombatBorder(void);
    void DrawCombatBorder(void);
    void SetupAndLoadObstacles(void);
    void MakeCreaturesVanish(void);
    void LowerDoor(void);
    void RaiseDoor(void);
    void TestRaiseDoor(void);
    int InCastle(int);
    int ShotIsThroughWall(int, int, int);
    void ShootMissile(int, int, int, int, float *, class icon *);
    void CombatSystemOptions(void);
    int AICheckRetreat(void);
    void DoCompAI(signed char);
    float GetModLichDamage(class army *, float);
    void DoLichShot(class army *);
    // HoMM1 AI masks take a byte side and return word bit masks.
    short GetShooterMask(signed char);
    int GetMirrorImageMask(int);
    short GetFlyerMask(signed char);
    int GetAllMask(int);
    short GetWalkerMask(signed char);
    int GetOutOfItMask(int);
    int GetTraitorMask(int);
    short GetBestArmy(signed char, short);
    short GetWorstArmy(signed char, short);
    short GetClosestArmy(class army *, signed char, short);
    unsigned long int GetStrength(signed char, short);
    signed char AttemptAttack(class army *, signed char, short);
    signed char AttemptAdjacentAttack(class army *);
    signed char WalkTowardArmyFront(class army *, signed char, short);
    signed char WalkTowardArmy(class army *, signed char, short);
};
#pragma pack(pop)

int ValidHex(int);
extern combatManager *gpCombatManager;
short WinCombatHandler(struct tag_message &);
short CombatSpecialHandler(struct tag_message &);
short HandleCastSpell(struct tag_message &);
// Spell-book hover help lines (0x00493a78).
extern char *cSpellHelp[];
// Captured artifacts shown page by page on the victory window.
extern signed char iMaxTransferArtifacts;
extern int iCurTransferArtifact;
// DoSurrender: gold the enemy hero asks for (0x004a4bac).
extern int giSurrenderCost;
// The queued combat action and its grid/extra arguments (0x004a4bc0..).
extern int giNextAction;
extern int giNextActionGridIndex;
extern int giNextActionExtra;
extern int giNextActionGridIndex2;
// Combat help lines for the auto-combat, skip and other controls.
extern char *cCombatHelp[];
// ProcessCombatMsg records the hero casting from the combat screen.
extern int giCurGeneral;
// A surrender ended the combat (0x004c6720).
extern signed char gbCombatSurrender;
// Fallback net player for a combat action broadcast (0x004c6710).
extern int giRemoteDefaultPlayer;
// Neighbour hex per combat hex and direction (0x004911c0), -1 off grid.
extern signed char gCombatAdjacency[45][6];
// Victory/defeat window texts (0x00493e48).
extern char *cBattleResults[];
extern signed char iTransferArtifacts[];
// Network combat: this machine controls the current side (0x004a4b98).
extern signed char gbThisNetHasControl;
// CheckWin flags a retreat victory (0x004c6d4c).
extern signed char gbRetreatWin;
// CheckHandleNet hands combat packets back while a battle is running.
extern signed char gbInCombat;
#endif // HOMM1_SOURCE_COMBATMANAGER_H

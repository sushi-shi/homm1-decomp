#ifndef HOMM1_SOURCE_COMBATMANAGER_H
#define HOMM1_SOURCE_COMBATMANAGER_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 149 methods, 3 own-virtual, 0 static data.

#include <BASE/baseManager.h>
#include <Domains.h>
#include <H1/Macros.h>
#include <SOURCE/army.h>
#include <SOURCE/hexcell.h>
#include <SOURCE/spellTypes.h>
#include <SOURCE/terrainTypes.h>

// forward declarations:
class army;
class armyGroup;
class hero;
class heroWindow;
class icon;
class town;
struct SBolt;
struct tag_message;

// Buka's CombatMessageCommand: GetCommand derives one from the hovered hex
// and DoCommand runs it (move, fly, shoot, own and opposing hero options,
// view, attack, spell book, retreat, surrender).
H1_ENUM_BEGIN(CombatMessageCommand)
    COMBAT_INVALID_COMMAND = -99,
    COMBAT_MESSAGE_COMMAND_DEFAULT = 0,
    COMBAT_MESSAGE_COMMAND_MOVE = 1,
    COMBAT_MESSAGE_COMMAND_FLY = 2,
    COMBAT_MESSAGE_COMMAND_SHOOT = 3,
    COMBAT_MESSAGE_COMMAND_OPTIONS = 4,
    COMBAT_MESSAGE_COMMAND_VIEW_INFO = 5,
    COMBAT_MESSAGE_COMMAND_ATTACK = 7,
    COMBAT_MESSAGE_COMMAND_CAST_SPELL = 10,
    COMBAT_MESSAGE_COMMAND_RETREAT = 11,
    COMBAT_MESSAGE_COMMAND_SURRENDER = 12,
    COMBAT_MESSAGE_COMMAND_OPPOSING_OPTIONS = 13
H1_ENUM_END(CombatMessageCommand)

// Buka's CombatAction: the queued giNextAction that the combat loop
// executes; DoCommand, the spell book and the skip button set it.
H1_ENUM_BEGIN(CombatAction)
    ACTION_NONE = 0,
    ACTION_CAST_SPELL = 1,
    ACTION_MOVE = 2,
    ACTION_SKIP_TURN = 3,
    ACTION_RETREAT = 4,
    ACTION_SURRENDER = 5,
    ACTION_ATTACK = 6
H1_ENUM_END(CombatAction)

// cmbtmous.mse frames: GetPointer returns the command's own frame (0..4)
// and maps the opposing-options command to the view pointer; DEFAULT is the
// plain arrow (hotspot 1,1) and ATTACK_FIRST + CombatHexDirection the sword
// pointing from that side (Buka COMBAT_POINTER_DEFAULT, POINTER_ATTACK_OFFSET).
H1_ENUM_BEGIN(CombatPointerCode)
    COMBAT_POINTER_VIEW = 5,
    COMBAT_POINTER_DEFAULT = 6,
    COMBAT_POINTER_ATTACK_FIRST = 7
H1_ENUM_END(CombatPointerCode)

// Two sides (attacker 0, defender 1) index m_armies and m_numArmies.
// The hex grid is nine columns by five rows (hex = row * 9 + column):
// DrawBackground and DrawFrame walk it row by row, army/AI/FLY code splits
// m_hex with % and / 9 and treats columns 0 and 8 as the side edges. In a
// siege the town wall stands in column 5 (DrawBackground draws it there,
// SpecialAttack tests shots across it, DoCompAI moves defenders to the
// column inside it).
H1_ENUM_CONST_BEGIN(CombatGridConstant)
    COMBAT_HEX_COUNT = 45,
    COMBAT_SIDE_ARMY_COUNT = 6,
    COMBAT_CASTLE_WALL_COLUMN = 5,
    // No stack slot: hexcell::m_occupantIndex of an empty hex and
    // army::m_targetIndex without a target (Buka COMBAT_AI_NO_ARMY).
    COMBAT_ARMY_INDEX_NONE = -1
H1_ENUM_CONST_END(CombatGridConstant)

// The battlefield view is the logical screen less SaveCombatBorder's
// twenty-row text bar: the background buffer copies 640x460 and the redraw
// extents clamp to its last row. Walk and FlyTo start the minimum extents
// at 640, past every view coordinate.
// Hexes are HEX_WIDTH x HEX_HEIGHT pixels; grid rows start FIELD_TOP pixels
// down and a hex's anchor y is row * HEX_HEIGHT + HEX_ORIGIN_Y (combatManager
// constructor, GetGridIndex, UpdateCombatArea, DrawBackground's wall strip,
// hexcell::DrawTower). DrawFrame draws the catapult with grid row
// CATAPULT_ROW and the attacker's and defender's tents with rows
// ATTACKER_HERO_ROW and DEFENDER_HERO_ROW.
H1_ENUM_CONST_BEGIN(CombatViewConstant)
    COMBAT_VIEW_HEIGHT = 460,
    COMBAT_EXTENT_MIN_START = 640,
    COMBAT_HEX_WIDTH = 78,
    COMBAT_HEX_HEIGHT = 80,
    COMBAT_FIELD_TOP = 60,
    COMBAT_HEX_ORIGIN_Y = 139,
    COMBAT_ATTACKER_HERO_ROW = 1,
    COMBAT_DEFENDER_HERO_ROW = 2,
    COMBAT_CATAPULT_ROW = 3
H1_ENUM_CONST_END(CombatViewConstant)

// Per-side draw sentinels: m_heroType and m_catapultFrame hold -1 for a side
// without a hero or catapult (DrawFrame skips the tent / catapult);
// ResetLimitCreature marks a dead stack's m_limitCreatureCount HIDDEN so
// DrawFrame never grows the redraw box for it.
H1_ENUM_CONST_BEGIN(CombatDrawStateConstant)
    COMBAT_HERO_TYPE_NONE = -1,
    COMBAT_CATAPULT_FRAME_NONE = -1,
    COMBAT_LIMIT_CREATURE_HIDDEN = -1,
    // m_wallDamage without damage frames to draw (hexcell::DrawWall).
    COMBAT_WALL_DAMAGE_NONE = -1,
    // m_wallFrame while no wall animation runs (CatAttack resets it).
    COMBAT_WALL_FRAME_NONE = -1
H1_ENUM_CONST_END(CombatDrawStateConstant)

// Combat AI tuning thresholds (Buka combatManager.h CombatAIConstant names
// with HoMM1's values): AICheckRetreat's artifact-value and army-strength
// tiers, experience divisor and difficulty scale; DoCompAI's rounded
// one-fifth strength; GetWorstArmy's starting strength; the unlimited
// speed WalkTowardArmy/WalkTowardArmyFront give a stack for path probes.
H1_ENUM_CONST_BEGIN(CombatAIConstant)
    COMBAT_AI_MAX_DIFFICULTY = 4,
    COMBAT_AI_STRENGTH_ROUNDING = 4,
    COMBAT_AI_STRENGTH_FRACTION = 5,
    COMBAT_AI_UNLIMITED_PATH_SPEED = 0x7f,
    COMBAT_AI_WORST_STRENGTH_LIMIT = 999999999,
    COMBAT_AI_MIN_ARTIFACT_VALUE = 1000,
    COMBAT_AI_MEDIUM_ARTIFACT_VALUE = 5000,
    COMBAT_AI_HIGH_ARTIFACT_VALUE = 10000,
    COMBAT_AI_RETREAT_TIER_1_THRESHOLD = 2500,
    COMBAT_AI_RETREAT_TIER_2_THRESHOLD = 5000,
    COMBAT_AI_RETREAT_TIER_3_THRESHOLD = 15000,
    COMBAT_AI_RETREAT_STRENGTH_DIVISOR = 20000,
    COMBAT_AI_RETREAT_TIER_4_THRESHOLD = 30000,
    COMBAT_AI_RETREAT_SCALED_PENALTY_THRESHOLD = 40000,
    COMBAT_AI_EXPERIENCE_DIVISOR = 200000,
    // DoCompAI's castle shooting estimate: the town's archers start at
    // BASE_ARCHERS, add ARCHERS_PER_DWELLING per dwelling and one per other
    // built structure, each worth ARCHER_STRENGTH to the defender.
    COMBAT_AI_CASTLE_BASE_ARCHERS = 5,
    COMBAT_AI_CASTLE_ARCHERS_PER_DWELLING = 4,
    COMBAT_AI_CASTLE_ARCHER_STRENGTH = 100
H1_ENUM_CONST_END(CombatAIConstant)

// DoCompAI's plan for the acting stack (Buka CombatAIConstant
// COMBAT_AI_ATTACK_*): shooters with shots left shoot, flyers fly, the rest
// walk.
H1_ENUM_BEGIN(CombatAIAttackPlan)
    COMBAT_AI_ATTACK_NONE = 0,
    COMBAT_AI_ATTACK_SHOOT = 1,
    COMBAT_AI_ATTACK_FLY = 2,
    COMBAT_AI_ATTACK_WALK = 3
H1_ENUM_END(CombatAIAttackPlan)

// combatManager::m_combatIcons slots, as LoadCombatResources fills them:
// the terrain's ground and obstacle icons, textbar.icn, catapult.icn,
// tent.icn, castle%02d.icn, cloud.icn, keep%02d.icn and spells.icn.
H1_ENUM_BEGIN(CombatIconSlot)
    COMBAT_ICON_GROUND = 0,
    COMBAT_ICON_TEXTBAR = 1,
    COMBAT_ICON_OBSTACLES = 2,
    COMBAT_ICON_CATAPULT = 3,
    COMBAT_ICON_TENT = 4,
    COMBAT_ICON_CASTLE = 5,
    COMBAT_ICON_CLOUD = 6,
    COMBAT_ICON_KEEP = 7,
    COMBAT_ICON_SPELLS = 8,
    COMBAT_ICON_COUNT = 9
H1_ENUM_END(CombatIconSlot)

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
    char m_unknown30[0xc];
    // Open loads kb.pal here and fades the screen in with it.
    class palette* m_combatPalette;
    hexcell m_hexCells[COMBAT_HEX_COUNT];
    // First grid row (0-4) UpdateCombatArea must redraw; 5 when clean.
    short m_gridUpdateRow;
    // DrawFrame skips the grid overlay while this is set (SetGridMode).
    signed char m_gridMode;
    // LoadIcons indexes the ground and obstacle tables by this terrain;
    // GetBackgroundName forces 6 for a graveyard field.
    signed char m_terrainType;
    // army::DoAttack sets it when the strike reaches the stack behind on a
    // downward diagonal; DrawFrame then grows each limited redraw box 60 rows
    // down. ResetLimitCreature clears it.
    signed char m_extendLimitDown;
    // SetupCombat keeps the defending town here as well.
    class town* m_originalCombatTown;
    // Open's small font; army::DrawToBuffer prints stack quantities with it.
    class font* m_smallFont;
    char m_unknown269[4];
    // SaveCombatBorder's copy of the twenty screen rows below the field.
    char* m_savedBorder;
    // Nine combat icons (retail loops 0..8 from +0x271): hexcell draws
    // ground (index), obstacles (2), towers (5) and walls (6); armies draw
    // the quantity box (1) and spell markers (8).
    class icon* m_combatIcons[COMBAT_ICON_COUNT];
    // Clean combat background: FlyTo and army::Walk restore the screen from it.
    class bitmap* m_backgroundBuffer;
    signed char m_backgroundDrawn;
    // GetBackgroundName reads the trigger of the cell the battle is on.
    class mapCell* m_battlefieldCell;
    // Per side: the town fought in. DoVictory gives the defender's winner
    // the castle bonus; AICheckRetreat never retreats from a town.
    class town* m_combatTowns[2];
    class hero* m_heroes[2];
    // Per side: the army group fought with (ViewArmy hands it to
    // game::ViewArmy).
    class armyGroup* m_armyGroups[2];
    // Set by a surrender (ProcessNextAction).
    signed char m_sideDefeated[2];
    // SetupCombat copies gbHumanPlayer per side; a bad-morale roll may spare
    // a computer side.
    char m_humanSide[2];
    signed char m_playerId[2];
    int m_experienceValue[2];
    signed char m_heroCastSpell[2];
    // Live stacks per side (CastMassSpell walks each side's armies).
    short m_numArmies[COMBAT_SIDE_COUNT];
    army m_armies[COMBAT_SIDE_COUNT][COMBAT_SIDE_ARMY_COUNT];
    signed char m_currentSide;
    signed char m_currentArmyIndex;
    signed char m_currentSpeed;
    signed char m_gridSelectionDisabled;
    signed char m_limitCreature;
    signed char m_limitCreatureHex;
    // Buka m_showArmyQuantities: army::DrawToBuffer draws the quantity box.
    signed char m_showArmyQuantities;
    signed char m_selectedHex;
    signed char m_directionTargetHex;
    signed char m_previousCommand;
    signed char m_currentCommand;
    // CatAttack animates the side's catapult through these frames.
    short m_catapultFrame[2];
    short m_catapultAttackCount[2];
    short m_catapultAttacksRemaining[2];
    short m_keepAttacksRemaining[2];
    // SetupCombat copies each hero's +0x1c byte (-1 without a hero).
    short m_heroType[2];
    short m_unknown6d9;
    short m_unknown6db;
    // Per side: the side fights from a castle. hexcell::DrawTower/DrawWall
    // mirror the castle art from side 1's flag.
    signed char m_castleSide[2];
    // Buka m_visitingHeroPresent: SetupCombat sets side 0 when the defending
    // town has a garrisoned hero.
    char m_visitingHeroPresent[2];
    // CatAttack's target row in the castle wall column.
    short m_catapultTarget;
    // CatAttack: 1 when the shot only damages the wall, 0 when it falls;
    // hexcell::DrawObstacle keeps the tower during the impact frames.
    signed char m_wallSurvives;
    short m_wallFrame;
    short m_wallDamage;
    signed char m_unknown6e8;
    // Per side: creatures the attacking ghosts (CREATURE_GHOST) killed; the
    // ghost stack grows by it after the strike. army::DoAttack stores and
    // reloads it with word moves indexed by side.
    short m_ghostKills[2];
    // LoadIcons loads the battlefield backdrop GetBackgroundName names;
    // DrawBackground draws it first.
    class bitmap* m_backgroundBitmap;
    // The combat screen window (Open's cmbtwin.bin); CombatMessage sets its
    // text widget (0xc).
    class heroWindow* m_combatWindow;
    char m_unknown6f5[4];
    short m_unknown6f9;
    // ProcessCombatMsg ignores message types outside this mask.
    short m_messageTypeMask;
    signed char m_sideRetreated[2];
    // Per stack draw state: ResetLimitCreature clears it (-1 for the dead)
    // and army::SpellEffect marks the stack it animates.
    int m_limitCreatureCount[2][5];
    // Buka passes these to DrawFrame as computeExtent/redrawExtent: the first
    // limits the redraw to the boxes of stacks in m_limitCreatureCount, the
    // second restores only the current extent from the background buffer.
    int m_computeExtent;
    int m_redrawExtent;
    // UpdateCombatArea does nothing until the combat window is up.
    int m_combatWindowOpen;
    class widget* m_winLoseBottomWidgets[15];
    class widget* m_winLoseBottomTextWidgets[15];
    // MoreTreesNear surveys the map around this adventure cell.
    int m_combatX;
    int m_combatY;
    // SetCombatDirections: attack direction per 15-degree mouse sector.
    signed char m_directionMap[24];
    signed char m_mouseDirection;
    signed char m_validDirectionCount;
    class heroWindow* m_winLoseWindow;
    signed char m_selectedSpell;
    // advManager::DoCombat returns and hands on this outcome byte.
    signed char m_combatResult;
    // --- constructors ---
    combatManager(void);
    // --- virtual methods (vtable order) ---
    virtual short Open(short) OVERRIDE;
    virtual void Close(void) OVERRIDE;
    virtual short Main(struct tag_message&) OVERRIDE;
    // --- methods ---
    void NoShowCombatLog(char*);
    void ClearCombatMessages(int);
    void CheckUpdateCombatMessages(void);
    // HoMM1 retail 0x00470aa9: text and a redraw flag (ret 8).
    void CombatMessage(char*, int);
    // HoMM1 retail 0x00470b5e: command help line (ret 4).
    void CombatMessage(H1_ENUM_PARAM(CombatMessageCommand, short));
    void ResetLimitCreature(void);
    void UpdateCombatArea(void);
    void SetupGridForArmy(class army*);
    // HoMM1 retail 0x00470a4f: word hex, byte direction, attributes
    // (ret 0xc); the upward directions also redraw the row above.
    void UpdateGridForMove(short, signed char, int);
    // HoMM1 retail 0x004709f0: word first hex, redraw flag (ret 8).
    void UpdateGrid(short, int);
    void DrawBackground(void);
    void UpdateMouseGrid(int, int);
    // HoMM1 retail 0x004711fb takes only the update flag (ret 4).
    void DrawFrame(signed char);
    // HoMM1 retail 0x00470f25: byte mode (ret 4).
    void SetGridMode(signed char);
    void DrawSmallView(int, int);
    // HoMM1 retail 0x00438310 returns its result in AL (ret 0xc).
    signed char ViewGeneral(int, int, int);
    // HoMM1 retail 0x00438a9f: army, side and a quick-view flag (ret 0xc).
    void ViewArmy(class army*, int, int);
    int HasValidSpellTarget(int);
    signed char ViewSpells(int);
    int FindResurrectArmyIndex(int, int, int);
    // HoMM1 retail 0x00415a2c: byte spell and hex, byte result (ret 8).
    signed char ValidSpellTarget(H1_ENUM_PARAM(SpellType, signed char), signed char);
    // HoMM1 retail 0x00415d1c: byte spell and hex (ret 8).
    void SpellMessage(H1_ENUM_PARAM(SpellType, signed char), signed char);
    // HoMM1 retail 0x00415e44: byte spell, hex, creature flag and teleport
    // destination (ret 0x10).
    void CastSpell(H1_ENUM_PARAM(SpellType, signed char), signed char, signed char, signed char);
    void DefaultSpell(signed char);
    // HoMM1 retail 0x00416c78: Cure (one side) and Dispel (both sides)
    // animation; byte side (2 = both) and cure-only flag (ret 8).
    void CastMassSpell(signed char, signed char);
    // HoMM1 retail 0x0041707f: cancels the side's spells after the mass
    // animation (ret 8).
    void CancelSideSpells(signed char, signed char);
    void Fireball(signed char);
    void MeteorShower(signed char);
    void ElementalStorm(void);
    void Armageddon(void);
    void TurnToStone(class army*);
    void BloodLustEffect(class army*, int);
    void Ripple(int);
    void Blur(int, int, int);
    void ResetBoltAngle(struct SBolt*);
    void DrawBolt(struct SBolt*, int);
    void AddBolt(struct SBolt*, int, int, int, int, int, int, int, int, int, int, int, int);
    void
    DoBolt(int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int);
    int GetNextChainLightningTarget(class army*, int);
    void ChainLightning(int, int);
    void VaporizeCreature(int, int);
    void RippleCreature(int, int, int);
    void MirrorImage(int);
    void SummonElemental(int, int);
    void DoLuck(int, int);
    void DoBlast(int, int);
    void Resurrect(int, int, int);
    int SpaceForElementalExists(void);
    void ShowSpellCastFailure(class army*, int);
    void ModifyDamageForArtifacts(long int*, int, class hero*, class hero*);
    void Earthquake(void);
    void ShowSpellMessage(int, int, class army*);
    signed char ValidHexToStandOn(int);
    void SetCombatDirections(int);
    void CheckSetMouseDirection(int, int, int);
    H1_ENUM_RETURN(CombatPointerCode, int) GetPointer(H1_ENUM_PARAM(CombatMessageCommand, int));
    int ProcessCombatMsg(struct tag_message&);
    int IsNegationSphereInEffect(void);
    void ResetRound(void);
    int CheckWin(struct tag_message*);
    signed char GetCommand(short);
    signed char RightClick(signed char);
    void DoCommand(signed char);
    void ClearWinLoseBottom(class heroWindow*);
    void ShowWinLoseArtifact(class heroWindow*, int);
    void ShowSkeletons(class heroWindow*);
    void ShowEagleEyeSpell(class heroWindow*);
    void ShowDeadArmies(class heroWindow*);
    void DoVictory(signed char);
    void DoLoseWindow(void);
    short DoSurrender(void);
    void CheckChangeSelector(void);
    void CheckCastleAttack(void);
    void CheckGetAIMove(void);
    void GetControl(void);
    void ResetMouse(void);
    short ProcessNextAction(struct tag_message&);
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
    void DetermineEffectOfSpell(int, int*, int*);
    int EffectSpellCreateCreature(int, int);
    int RawEffectSpellInfluence(class army*, int);
    void ClearEffects(void);
    void NextPos(int*);
    int FirstArmy(int, int, int*);
    int FirstResurrectable(int, int*, int);
    // HoMM1 retail 0x00437aa1 (ret 0xc), 0x00437d14 (ret 8) and 0x00437e0d
    // (ret 0x10): DetermineEffectOfSpell passes the effect, then a side and
    // flag, a hex, or the spell, base damage and hex.
    void EffectSpellCure(int*, int, signed char);
    void EffectSpellResurrect(int*, int);
    void EffectSpellDamage(int*, int, int, int);
    void CombineGroups(class armyGroup*, class armyGroup*);
    void SetupCombat(
        int,
        int,
        class hero*,
        class armyGroup*,
        class town*,
        class hero*,
        class armyGroup*,
        int,
        int,
        int
    );
    void InitNonVisualVars(void);
    void SetupAdjacencyArray(void);
    // HoMM1 retail 0x0044c103: byte side (ret 4).
    void UpdateArmyGroup(signed char);
    void GenerateMap(void);
    char* GetBackgroundName(void);
    signed char MoreTreesNear(void);
    // HoMM1 retail 0x0044e7f2: no callers; rebuilds the field and redraws.
    void RegenerateField(void);
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
    // HoMM1 retail 0x0044f3cb: byte side (ret 4).
    int ExperienceValueOfStack(signed char);
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
    void ShootMissile(int, int, int, int, float*, class icon*);
    void CombatSystemOptions(void);
    int AICheckRetreat(void);
    // HoMM1 retail 0x00464ca3: byte side (ret 4).
    void DoCompAI(signed char);
    float GetModLichDamage(class army*, float);
    void DoLichShot(class army*);
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
    short GetClosestArmy(class army*, signed char, short);
    unsigned long int GetStrength(signed char, short);
    signed char AttemptAttack(class army*, signed char, short);
    signed char AttemptAdjacentAttack(class army*);
    signed char WalkTowardArmyFront(class army*, signed char, short);
    signed char WalkTowardArmy(class army*, signed char, short);
};
#pragma pack(pop)

int ValidHex(int);
short WinCombatHandler(struct tag_message&);
short CombatSpecialHandler(struct tag_message&);
short HandleCastSpell(struct tag_message&);
// HandleCastSpell: the hex under the spell pointer (0x0048f2b0) and the
// teleport second-click state (0x0048f28c).
extern signed char bInTeleportGetDest;
// The loaded combat effect icon's file id (0x004c6d64).
extern short gCurLoadedSpellFileId;
// Frame of the mass-spell glow drawn by DrawFrame (0x004c78b4).
// Stale alias of giSpellEffectFrame (0x4c78b4): unreferenced, kept so later symbol handles stay put.
extern short giCombatFxFrame;
// Spell-book hover help lines (0x00493a78).
extern char* cSpellHelp[];
// Captured artifacts shown page by page on the victory window.
extern signed char iMaxTransferArtifacts;
extern int iCurTransferArtifact;
// DoSurrender: gold the enemy hero asks for (0x004a4bac).
extern int giSurrenderCost;
// The queued combat action and its grid/extra arguments (0x004a4bc0..).
extern H1_ENUM_STORAGE(CombatAction, int) giNextAction;
extern int giNextActionGridIndex;
extern int giNextActionExtra;
extern int giNextActionGridIndex2;
// cCombatMessage indices, the command help lines CombatMessage(short)
// prints: "", "Move %s here.", "Fly %s here.", "Attack %s", "Shoot %s(%d
// shot%s left)", "General's Options", "View Opposing General", "View %s
// info." and "No shots left!".
H1_ENUM_BEGIN(CombatMessageText)
    COMBAT_TEXT_NONE = 0,
    COMBAT_TEXT_MOVE = 1,
    COMBAT_TEXT_FLY = 2,
    COMBAT_TEXT_ATTACK = 3,
    COMBAT_TEXT_SHOOT = 4,
    COMBAT_TEXT_GENERALS_OPTIONS = 5,
    COMBAT_TEXT_VIEW_OPPOSING_GENERAL = 6,
    COMBAT_TEXT_VIEW_INFO = 7,
    COMBAT_TEXT_NO_SHOTS = 8,
    COMBAT_TEXT_COUNT = 9
H1_ENUM_END(CombatMessageText)
// Command help lines for CombatMessage(short) (0x00493b38).
extern char* cCombatMessage[];
// Combat help lines for the auto-combat, skip and other controls.
extern char* cCombatHelp[];
// ProcessCombatMsg records the hero casting from the combat screen.
extern int giCurGeneral;
// Fallback net player for a combat action broadcast (0x004c6710).
// Stale alias of giHostGamePos (0x4c6710): unreferenced, kept so later symbol handles stay put.
extern int giRemoteDefaultPlayer;
// Neighbour hex per combat hex and direction (0x004911c0), -1 off grid.
extern signed char gCombatAdjacency[45][6];
// Victory/defeat window texts (0x00493e48).
extern char* cBattleResults[];
extern signed char iTransferArtifacts[];
// Network combat: this machine controls the current side (0x004a4b98).
extern signed char gbThisNetHasControl;
// CheckHandleNet hands combat packets back while a battle is running.
extern signed char gbInCombat;
// Battlefield backdrops per combat terrain (CMBTMGR data, 0x00490db0); the
// ground and obstacle tables are in X_GLOBAL.h.
#endif // HOMM1_SOURCE_COMBATMANAGER_H

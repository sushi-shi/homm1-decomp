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
    i8 sender;
    i32 id;
    i8 type;
    i8 command;
    i16 payloadSize;
    union {
        struct {
            i32 nextAction;
            i32 nextActionExtra;
            i32 nextActionGridIndex;
            i32 nextActionGridIndex2;
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
    bool m_restoreMusicSuspension;
    bool m_restoreSampleSuspension;
    i32 m_savedMusicTrack;
    // Open loads kb.pal here and fades the screen in with it.
    class palette* m_combatPalette;
    hexcell m_hexCells[COMBAT_HEX_COUNT];
    // First grid row (0-4) UpdateCombatArea must redraw; 5 when clean.
    i16 m_gridUpdateRow;
    // DrawFrame skips the grid overlay while this is set (SetGridMode).
    i8 m_gridMode;
    // LoadIcons indexes the ground and obstacle tables by this terrain;
    // GetBackgroundName forces 6 for a graveyard field.
    i8 m_terrainType;
    // army::DoAttack sets it when the strike reaches the stack behind on a
    // downward diagonal; DrawFrame then grows each limited redraw box 60 rows
    // down. ResetLimitCreature clears it.
    i8 m_extendLimitDown;
    // SetupCombat keeps the defending town here as well.
    class town* m_originalCombatTown;
    // Open's small font; army::DrawToBuffer prints stack quantities with it.
    class font* m_smallFont;
    // SaveCombatBorder's copy of the twenty screen rows below the field.
    char* m_savedBorder;
    // Nine combat icons (retail loops 0..8 from +0x271): hexcell draws
    // ground (index), obstacles (2), towers (5) and walls (6); armies draw
    // the quantity box (1) and spell markers (8).
    class icon* m_combatIcons[COMBAT_ICON_COUNT];
    // Clean combat background: FlyTo and army::Walk restore the screen from it.
    class bitmap* m_backgroundBuffer;
    i8 m_backgroundDrawn;
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
    i8 m_sideDefeated[2];
    // SetupCombat copies gbHumanPlayer per side; a bad-morale roll may spare
    // a computer side.
    char m_humanSide[2];
    i8 m_playerId[2];
    i32 m_experienceValue[2];
    i8 m_heroCastSpell[2];
    // Live stacks per side (CastMassSpell walks each side's armies).
    i16 m_numArmies[COMBAT_SIDE_COUNT];
    army m_armies[COMBAT_SIDE_COUNT][COMBAT_SIDE_ARMY_COUNT];
    i8 m_currentSide;
    i8 m_currentArmyIndex;
    i8 m_currentSpeed;
    i8 m_gridSelectionDisabled;
    i8 m_limitCreature;
    i8 m_limitCreatureHex;
    // Buka m_showArmyQuantities: army::DrawToBuffer draws the quantity box.
    i8 m_showArmyQuantities;
    i8 m_selectedHex;
    i8 m_directionTargetHex;
    i8 m_previousCommand;
    i8 m_currentCommand;
    // CatAttack animates the side's catapult through these frames.
    i16 m_catapultFrame[2];
    i16 m_catapultAttackCount[2];
    i16 m_catapultAttacksRemaining[2];
    i16 m_keepAttacksRemaining[2];
    // SetupCombat copies each hero's +0x1c byte (-1 without a hero).
    i16 m_heroType[2];
    i16 m_unknown6d9;
    i16 m_unknown6db;
    // Per side: the side fights from a castle. hexcell::DrawTower/DrawWall
    // mirror the castle art from side 1's flag.
    i8 m_castleSide[2];
    // Buka m_visitingHeroPresent: SetupCombat sets side 0 when the defending
    // town has a garrisoned hero.
    char m_visitingHeroPresent[2];
    // CatAttack's target row in the castle wall column.
    i16 m_catapultTarget;
    // CatAttack: 1 when the shot only damages the wall, 0 when it falls;
    // hexcell::DrawObstacle keeps the tower during the impact frames.
    i8 m_wallSurvives;
    i16 m_wallFrame;
    i16 m_wallDamage;
    i8 m_unknown6e8;
    // Per side: creatures the attacking ghosts (CREATURE_GHOST) killed; the
    // ghost stack grows by it after the strike. army::DoAttack stores and
    // reloads it with word moves indexed by side.
    i16 m_ghostKills[2];
    // LoadIcons loads the battlefield backdrop GetBackgroundName names;
    // DrawBackground draws it first.
    class bitmap* m_backgroundBitmap;
    // The combat screen window (Open's cmbtwin.bin); CombatMessage sets its
    // text widget (0xc).
    class heroWindow* m_combatWindow;
    char m_unused6f5[4];
    i16 m_unknown6f9;
    // ProcessCombatMsg ignores message types outside this mask.
    i16 m_messageTypeMask;
    i8 m_sideRetreated[2];
    // Per stack draw state: ResetLimitCreature clears it (-1 for the dead)
    // and army::SpellEffect marks the stack it animates.
    i32 m_limitCreatureCount[2][5];
    // Buka passes these to DrawFrame as computeExtent/redrawExtent: the first
    // limits the redraw to the boxes of stacks in m_limitCreatureCount, the
    // second restores only the current extent from the background buffer.
    i32 m_computeExtent;
    i32 m_redrawExtent;
    // UpdateCombatArea does nothing until the combat window is up.
    i32 m_combatWindowOpen;
    class widget* m_winLoseBottomWidgets[15];
    class widget* m_winLoseBottomTextWidgets[15];
    // MoreTreesNear surveys the map around this adventure cell.
    i32 m_combatX;
    i32 m_combatY;
    // SetCombatDirections: attack direction per 15-degree mouse sector.
    i8 m_directionMap[24];
    i8 m_mouseDirection;
    i8 m_validDirectionCount;
    class heroWindow* m_winLoseWindow;
    i8 m_selectedSpell;
    // advManager::DoCombat returns and hands on this outcome byte.
    i8 m_combatResult;
    // --- constructors ---
    combatManager(void);
    // --- virtual methods (vtable order) ---
    virtual i16 Open(i16 priority) OVERRIDE;
    virtual void Close(void) OVERRIDE;
    virtual i16 Main(struct tag_message& message) OVERRIDE;
    // --- methods ---
    void NoShowCombatLog(char*);
    void ClearCombatMessages(i32 force);
    void CheckUpdateCombatMessages(void);
    // HoMM1 retail 0x00470aa9: text and a redraw flag (ret 8).
    void CombatMessage(char* text, i32 updateScreen);
    // HoMM1 retail 0x00470b5e: command help line (ret 4).
    void CombatMessage(H1_ENUM_PARAM(CombatMessageCommand, i16) messageType);
    void ResetLimitCreature(void);
    void UpdateCombatArea(void);
    void SetupGridForArmy(class army* armyPointer);
    // Buka RVA 0x236c0: word hex, byte direction, word attributes
    // (ret 0xc); the upward directions also redraw the row above.
    void UpdateGridForMove(i16 hex, i8 direction, i16 attributes);
    // Buka RVA 0x23670: word hex and unused word attributes (ret 8).
    void UpdateGrid(i16 hex, i16);
    void DrawBackground(void);
    void UpdateMouseGrid(i32 hexIndex, i32 forceUpdate);
    // HoMM1 retail 0x004711fb takes only the update flag (ret 4).
    void DrawFrame(i8 updateScreen);
    // Buka RVA 0x23acc: byte mode (ret 4).
    void SetGridMode(i8 mode);
    void DrawSmallView(i32 viewIndex, i32 updateScreen);
    // HoMM1 retail 0x00438310 returns its result in AL (ret 0xc).
    i8 ViewGeneral(i32 side, i32 allowActions, i32 quickView);
    // HoMM1 retail 0x00438a9f: army, side and a quick-view flag (ret 0xc).
    void ViewArmy(class army* viewedArmy, i32 side, i32 quickView);
    i32 HasValidSpellTarget(i32 spell);
    i8 ViewSpells(i32);
    i32 FindResurrectArmyIndex(i32 side, i32 spell, i32 hex);
    // HoMM1 retail 0x0040e04c: byte spell and hex, byte result (ret 8).
    i8 ValidSpellTarget(H1_ENUM_PARAM(SpellType, i8) spell, i8 hex);
    // HoMM1 retail 0x0040e33c: byte spell and hex (ret 8).
    void SpellMessage(H1_ENUM_PARAM(SpellType, i8) spell, i8 hex);
    // HoMM1 retail 0x0040e464: byte spell, hex, creature flag and teleport
    // destination (ret 0x10).
    void
    CastSpell(H1_ENUM_PARAM(SpellType, i8) spell, i8 targetHex, i8 castByCreature, i8 teleportDest);
    void DefaultSpell(i8 targetHex);
    // HoMM1 retail 0x0040f298: Cure (one side) and Dispel (both sides)
    // animation; byte side (2 = both) and cure-only flag (ret 8).
    void CastMassSpell(i8 castSide, i8 cureOnly);
    // HoMM1 retail 0x0040f69f: cancels the side's spells after the mass
    // animation (ret 8).
    void CancelSideSpells(i8 side, i8 cureOnly);
    void Fireball(i8 targetHex);
    void MeteorShower(i8 targetHex);
    void ElementalStorm(void);
    void Armageddon(void);
    void TurnToStone(class army* target);
    void BloodLustEffect(class army* target, i32 effect);
    void Ripple(i32 strength);
    void Blur(i32 redAdjust, i32 greenAdjust, i32 blueAdjust);
    void ResetBoltAngle(struct SBolt* bolt);
    void DrawBolt(struct SBolt* bolt, i32 stepCount);
    void AddBolt(
        struct SBolt* bolt,
        i32 startX,
        i32 startY,
        i32 endX,
        i32 endY,
        i32 branchDistance,
        i32 startWidth,
        i32 endWidth,
        i32 colorMode,
        i32 minAngle,
        i32 maxAngle,
        i32 angleDistance,
        i32 forceAngle
    );
    void DoBolt(
        i32 managePointer,
        i32 startX,
        i32 startY,
        i32 endX,
        i32 endY,
        i32 branchDistance,
        i32 branchLength,
        i32 startWidth,
        i32 endWidth,
        i32 colorMode,
        i32 minAngle,
        i32 maxAngle,
        i32 angleDistance,
        i32 unusedParameter,
        i32 forceAngle,
        i32 frameDelay,
        i32 brightenPalette
    );
    i32 GetNextChainLightningTarget(class army* source, i32 requireWorks);
    void ChainLightning(i32 targetHex, i32 spellPower);
    void VaporizeCreature(i32 side, i32 armyIndex);
    void RippleCreature(i32 side, i32 armyIndex, i32 mode);
    void MirrorImage(i32 targetHex);
    void SummonElemental(i32 monsterType, i32 spellPower);
    void DoLuck(i32 side, i32 armyIndex);
    void DoBlast(i32 targetHex, i32 spell);
    void Resurrect(i32 spell, i32 targetHex, i32 spellPower);
    i32 SpaceForElementalExists(void);
    void ShowSpellCastFailure(class army*, i32);
    void
    ModifyDamageForArtifacts(i32* damage, i32 spell, class hero* attacker, class hero* defender);
    void Earthquake(void);
    void ShowSpellMessage(i32 castByCreature, i32 spell, class army* target);
    i8 ValidHexToStandOn(i32 hex);
    void SetCombatDirections(i32 targetHex);
    void CheckSetMouseDirection(i32 mouseX, i32 mouseY, i32 targetHex);
    H1_ENUM_RETURN(CombatPointerCode, i32) GetPointer(H1_ENUM_PARAM(CombatMessageCommand, i32) command);
    i32 ProcessCombatMsg(struct tag_message& message);
    i32 IsNegationSphereInEffect(void);
    void ResetRound(void);
    i32 CheckWin(struct tag_message* message);
    i8 GetCommand(i16 hex);
    i8 RightClick(i8 hex);
    void DoCommand(i8 command);
    void ClearWinLoseBottom(class heroWindow* window);
    void ShowWinLoseArtifact(class heroWindow* window, i32 artifact);
    void ShowSkeletons(class heroWindow* window);
    void ShowEagleEyeSpell(class heroWindow* window);
    void ShowDeadArmies(class heroWindow* window);
    void DoVictory(i8 winningSide);
    void DoLoseWindow(void);
    i16 DoSurrender(void);
    void CheckChangeSelector(void);
    void CheckCastleAttack(void);
    void CheckGetAIMove(void);
    void GetControl(void);
    void ResetMouse(void);
    i16 ProcessNextAction(struct tag_message& message);
    void ResetCyclingCreatures(void);
    void ResetCycleTimers(void);
    void CycleCombatScreen(void);
    void SetCombatViewArmySmallLevel(i32 level);
    void SetCombatGrid(i32 showGrid, i32 showMouseHex, i32 shadeLevel);
    void AddArmy(i32 side, i32 monsterType, i32 quantity, i32 hex, i32 flags, i32 animate);
    void SetupSmallView(void);
    void ViewBallista(i32 quickView);
    // HoMM1 retail 0x00437010: byte side (ret 4).
    i32 DoSpellAI(i8 side);
    void DetermineEffectOfSpell(i32 spell, i32* bestEffect, i32* bestHex);
    i32 EffectSpellCreateCreature(i32 hex, i32 spell);
    i32 RawEffectSpellInfluence(class army* target, i32 spell);
    void ClearEffects(void);
    void NextPos(i32* hex);
    i32 FirstArmy(i32 startHex, i32 side, i32* hex);
    i32 FirstResurrectable(i32 startHex, i32* hex, i32 spell);
    // HoMM1 retail 0x00437aa1 (ret 0xc), 0x00437d14 (ret 8) and 0x00437e0d
    // (ret 0x10): DetermineEffectOfSpell passes the effect, then a side and
    // flag, a hex, or the spell, base damage and hex.
    void EffectSpellCure(i32* effect, i32 targetSide, i8 cure);
    void EffectSpellResurrect(i32* effect, i32 hex);
    void EffectSpellDamage(i32* effect, i32 spell, i32 damagePerPower, i32 targetHex);
    void CombineGroups(class armyGroup* from, class armyGroup* to);
    void SetupCombat(
        i32 mapX,
        i32 mapY,
        class hero* attackerHero,
        class armyGroup* attackerGroup,
        class town* defenderTown,
        class hero* defenderHero,
        class armyGroup* defenderGroup,
        i32 combatX,
        i32 combatY,
        i32 randomSeed
    );
    void InitNonVisualVars(void);
    void SetupAdjacencyArray(void);
    // HoMM1 retail 0x0044c103: byte side (ret 4).
    void UpdateArmyGroup(i8 side);
    void GenerateMap(void);
    char* GetBackgroundName(void);
    i8 MoreTreesNear(void);
    // HoMM1 retail 0x0044e7f2: no callers; rebuilds the field and redraws.
    void RegenerateField(void);
    void LoadIcons(void);
    void FreeIcons(void);
    void LoadArmies(void);
    void FreeArmies(void);
    i16 GetGridIndex(i16 x, i16 y);
    void CheckApplyGoodMorale(i32 side, i32 index);
    i32 CheckApplyBadMorale(i32 side, i32 index);
    // HoMM1 returns the found flag in AL.
    i8 GetNextArmy(i32 checkMorale);
    // HoMM1 retail 0x0044d9ca: byte side, byte result.
    i8 IsWinner(i8 side);
    void CatAttack(i8 side);
    // HoMM1 has a single keep (retail 0x0044e840, plain ret).
    void KeepAttack(void);
    // HoMM1 retail 0x0044f3cb: byte side (ret 4).
    i32 ExperienceValueOfStack(i8 side);
    void ResetHitByCreature(void);
    void SaveCombatBorder(void);
    void DrawCombatBorder(void);
    void SetupAndLoadObstacles(void);
    void MakeCreaturesVanish(void);
    void LowerDoor(void);
    void RaiseDoor(void);
    void TestRaiseDoor(void);
    i32 InCastle(i32 hex);
    i32 ShotIsThroughWall(i32 side, i32 sourceHex, i32 targetHex);
    void ShootMissile(
        i32 sourceX,
        i32 sourceY,
        i32 targetX,
        i32 targetY,
        float* directionAngles,
        class icon* missileIcon
    );
    void CombatSystemOptions(void);
    i32 AICheckRetreat(void);
    // HoMM1 retail 0x00464ca3: byte side (ret 4).
    void DoCompAI(i8);
    float GetModLichDamage(class army* target, float damage);
    void DoLichShot(class army* lich);
    // HoMM1 AI masks take a byte side and return word bit masks.
    i16 GetShooterMask(i8 side);
    i32 GetMirrorImageMask(i32 side);
    i16 GetFlyerMask(i8 side);
    i32 GetAllMask(i32 side);
    i16 GetWalkerMask(i8 side);
    i32 GetOutOfItMask(i32 side);
    i32 GetTraitorMask(i32 side);
    i16 GetBestArmy(i8 side, i16 mask);
    i16 GetWorstArmy(i8 side, i16 mask);
    i16 GetClosestArmy(class army* currentArmy, i8 side, i16 mask);
    u32 GetStrength(i8 side, i16 mask);
    i8 AttemptAttack(class army* currentArmy, i8 side, i16 mask);
    i8 AttemptAdjacentAttack(class army* currentArmy);
    i8 WalkTowardArmyFront(class army* currentArmy, i8 side, i16 mask);
    i8 WalkTowardArmy(class army* currentArmy, i8 side, i16 mask);
};
#pragma pack(pop)

i32 ValidHex(i32 hex);
i16 WinCombatHandler(struct tag_message& message);
i16 CombatSpecialHandler(struct tag_message& message);
i16 HandleCastSpell(struct tag_message& message);
// HandleCastSpell: the hex under the spell pointer (0x004906b4) and the
// teleport second-click state (0x00490690).
extern i8 gInTeleportGetDest;
// Frame of the mass-spell glow drawn by DrawFrame (0x004c78b4).
// Stale alias of gSpellEffectFrame (0x4c78b4): unreferenced, kept so later symbol handles stay put.
extern i16 giCombatFxFrame;
// Captured artifacts shown page by page on the victory window.
extern i8 iMaxTransferArtifacts;
extern i32 iCurTransferArtifact;
// DoSurrender: gold the enemy hero asks for (0x004a4bac).
extern i32 giSurrenderCost;
// The queued combat action and its grid/extra arguments (0x004a4bc0..).
extern H1_ENUM_STORAGE(CombatAction, i32) giNextAction;
extern i32 giNextActionGridIndex;
extern i32 giNextActionExtra;
extern i32 giNextActionGridIndex2;
// Queue a move (or attack) toward a hex: the
// action is stored before the hex expression is evaluated; other action
// fields stay with the caller.
#define SET_NEXT_COMBAT_MOVE(hex) (giNextAction = ACTION_MOVE, giNextActionGridIndex = (hex))
// gCombatMessage indices, the command help lines CombatMessage(short)
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
// Fallback net player for a combat action broadcast (0x004c6710).
// Stale alias of giHostGamePos (0x4c6710): unreferenced, kept so later symbol handles stay put.
extern i32 giRemoteDefaultPlayer;
extern i8 iTransferArtifacts[];
// Network combat: this machine controls the current side (0x004a4b98).
extern i8 gbThisNetHasControl;
// Moved from COMMAND.cpp.
// gCombatHelp rows ProcessCombatMsg shows when the pointer is off the grid:
// over the auto-combat strip (left), the skip strip (right), or neither.
H1_ENUM_BEGIN(CombatHelpText)
    COMBAT_HELP_AUTO_COMBAT = 0,
    COMBAT_HELP_SKIP_UNIT = 1,
    COMBAT_HELP_NONE = 2
H1_ENUM_END(CombatHelpText)

// gBattleResults rows (DoVictory's win texts and DoLoseWindow's loss texts):
// the outcome lines, then the experience award with or without level-ups.
H1_ENUM_BEGIN(BattleResultText)
    BATTLE_RESULT_ENEMY_SURRENDERED = 0,
    BATTLE_RESULT_ENEMY_FLED = 1,
    BATTLE_RESULT_VICTORY = 2,
    BATTLE_RESULT_EXPERIENCE = 3,
    BATTLE_RESULT_HERO_SURRENDERS = 4,
    BATTLE_RESULT_HERO_FLEES = 5,
    BATTLE_RESULT_HERO_DEFEATED = 6,
    BATTLE_RESULT_FORCES_SURRENDER = 7,
    BATTLE_RESULT_FORCES_FLEE = 8,
    BATTLE_RESULT_FORCES_DEFEATED = 9,
    BATTLE_RESULT_EXPERIENCE_AND_LEVELS = 10
H1_ENUM_END(BattleResultText)

// win/losecmbt.bin widget ids: the animation, the result text, and the
// bottom panel ShowWinLoseArtifact (captured artifact) or ShowDeadArmies
// (casualties: icon/count ids are FIRST + side * ARMY_GROUP_SLOT_COUNT + slot,
// with the count id doubling as the side's "None" line) fills in.
H1_ENUM_BEGIN(CombatWinLoseControl)
    WIN_LOSE_ANIMATION = 1,
    WIN_LOSE_RESULT_TEXT = 0x65,
    WIN_LOSE_CASUALTY_ICON_FIRST = 0x7d0,
    WIN_LOSE_ARTIFACT_BACKGROUND = 0x7d1,
    WIN_LOSE_ARTIFACT_ICON = 0x7d2,
    WIN_LOSE_CASUALTY_TEXT_FIRST = 0x834,
    WIN_LOSE_ARTIFACT_NAME = 0x835,
    WIN_LOSE_CASUALTY_HEADING = 0x83e
H1_ENUM_END(CombatWinLoseControl)

// m_winLoseBottomTextWidgets slots: side * ARMY_GROUP_SLOT_COUNT + slot for
// the casualty counts, then the two side headings and the casualty title.
// DoVictory's experience line buffer (Buka 2.1 VICTORY_EXPERIENCE_TEXT_SIZE;
// retail's frame places message directly above a 152-byte array).
H1_ENUM_CONST_BEGIN(CombatVictoryConstant)
    COMBAT_VICTORY_EXPERIENCE_TEXT_SIZE = 152
H1_ENUM_CONST_END(CombatVictoryConstant)

H1_ENUM_CONST_BEGIN(CombatWinLoseSlot)
    WIN_LOSE_SLOT_SIDE_HEADING_FIRST = 10,
    WIN_LOSE_SLOT_CASUALTY_TITLE = 12,
    WIN_LOSE_SLOT_COUNT = 15
H1_ENUM_CONST_END(CombatWinLoseSlot)

// surrendr.bin widget ids DoSurrender fills (the victor's portrait, the offer).
H1_ENUM_BEGIN(SurrenderControl)
    SURRENDER_PORTRAIT = 1,
    SURRENDER_TEXT = 2
H1_ENUM_END(SurrenderControl)

// SetCombatDirections' rear hex for a one-hex stack: no rear hex to check
// (ValidHexToStandOn accepts it; CheckSetMouseDirection's backHex default).
H1_ENUM_CONST_BEGIN(CombatRearHexConstant)
    COMBAT_REAR_HEX_UNUSED = -2
H1_ENUM_CONST_END(CombatRearHexConstant)

// Combat-window widget ids ProcessCombatMsg handles: the battlefield (0x40,
// Buka CombatControlId CONTROL_MAIN_BUTTON; ResetMouse hovers it), the button
// that stops grid selection and hides the pointer, and the skip-turn button
// that queues ACTION_SKIP_TURN.
H1_ENUM_BEGIN(CombatControlId)
    COMBAT_CONTROL_DISABLE_SELECTION = 2,
    COMBAT_CONTROL_SKIP_TURN = 8,
    COMBAT_CONTROL_FIELD = 0x40
H1_ENUM_END(CombatControlId)

// Moved from DRAWING.cpp.
// clang-format off
// cmbtwin.bin's status line: CombatMessage sets the text widget (id 12),
// redraws widgets 2..12 of the text bar and blits the bar's screen rectangle.
H1_ENUM_CONST_BEGIN(CombatStatusLineConstant)
    COMBAT_STATUS_FIRST_CONTROL = 2,
    COMBAT_STATUS_TEXT_CONTROL = 0xc,
    COMBAT_STATUS_X = 0x30,
    COMBAT_STATUS_Y = 0x1cc,
    COMBAT_STATUS_WIDTH = 0x21f,
    COMBAT_STATUS_HEIGHT = 0x14
H1_ENUM_CONST_END(CombatStatusLineConstant)

// Moved from CMBTMGR.cpp.
// gCombatBkgNames rows: GetBackgroundName picks one per terrain (forest or
// mountain variant by MoreTreesNear), the boat for water and the graveyard.
H1_ENUM_BEGIN(CombatBackground)
    COMBAT_BACKGROUND_GRASS_FOREST = 0,
    COMBAT_BACKGROUND_GRASS_MOUNTAIN = 1,
    COMBAT_BACKGROUND_SNOW_FOREST = 2,
    COMBAT_BACKGROUND_SNOW_MOUNTAIN = 3,
    COMBAT_BACKGROUND_SWAMP = 4,
    COMBAT_BACKGROUND_LAVA = 5,
    COMBAT_BACKGROUND_DESERT = 6,
    COMBAT_BACKGROUND_DIRT_FOREST = 7,
    COMBAT_BACKGROUND_DIRT_MOUNTAIN = 8,
    COMBAT_BACKGROUND_BOAT = 9,
    COMBAT_BACKGROUND_GRAVEYARD = 10,
    COMBAT_BACKGROUND_COUNT = 11
H1_ENUM_END(CombatBackground)

// Moved from SPELLS.cpp.
// spelmous.mse frames: HandleCastSpell shows the selected SpellType's own
// frame over a valid target and frame 19, after the combat spells, otherwise.
H1_ENUM_BEGIN(SpellPointerFrame)
    SPELL_POINTER_NO_TARGET = 19
H1_ENUM_END(SpellPointerFrame)

// Moved from SPELLAI.cpp.
// DetermineEffectOfSpell's target walk (Buka 2.1 SPELLAI.cpp
// CombatSpellAITargetMode; HoMM1 numbers its four modes in this order): one
// global evaluation, every area position, or each friendly / enemy stack.
H1_ENUM_BEGIN(CombatSpellAITargetMode)
    SPELL_AI_GLOBAL = 0,
    SPELL_AI_AREA = 1,
    SPELL_AI_FRIENDLY = 2,
    SPELL_AI_ENEMY = 3
H1_ENUM_END(CombatSpellAITargetMode)

// Grid rows of the castle wall's two sections either side of the gate:
// CatAttack's targets, and the rows from which an archer's shot can pass
// the wall on even terms (army::SpecialAttack).
H1_ENUM_CONST_BEGIN(CastleWallRow)
    COMBAT_UPPER_WALL_ROW = 1,
    COMBAT_LOWER_WALL_ROW = 3
H1_ENUM_CONST_END(CastleWallRow)

// Combat drawing frames: CatAttack's wall collapses at COLLAPSE_FRAME; the
// attacker's catapult arm shows its released image at RELEASE_FRAME;
// RandomizeObstacles redraws obstacle frame LAND_ONLY as 0 on water and lava.
H1_ENUM_CONST_BEGIN(CombatDrawFrameConstant)
    COMBAT_WALL_COLLAPSE_FRAME = 5,
    COMBAT_CATAPULT_RELEASE_FRAME = 7,
    COMBAT_OBSTACLE_LAND_ONLY_FRAME = 2
H1_ENUM_CONST_END(CombatDrawFrameConstant)

// CheckSetMouseDirection's cursor sectors around the target hex; each adds
// the steepness band to pick one of the 24 direction cursors.
H1_ENUM_CONST_BEGIN(CombatCursorSector)
    COMBAT_CURSOR_SECTOR_RIGHT_UP = 0,
    COMBAT_CURSOR_SECTOR_RIGHT_DOWN = 6,
    COMBAT_CURSOR_SECTOR_LEFT_DOWN = 12,
    COMBAT_CURSOR_SECTOR_LEFT_UP = 18
H1_ENUM_CONST_END(CombatCursorSector)

#endif // HOMM1_SOURCE_COMBATMANAGER_H

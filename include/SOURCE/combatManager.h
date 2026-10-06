#ifndef HOMM1_SOURCE_COMBATMANAGER_H
#define HOMM1_SOURCE_COMBATMANAGER_H

#include <BASE/audioTypes.h>
#include <BASE/baseManager.h>
#include <BASE/message.h>
#include <Domains.h>
#include <H1/Macros.h>
#include <SOURCE/army.h>
#include <SOURCE/artifactTypes.h>
#include <SOURCE/hexcell.h>
#include <SOURCE/REMOTE.h>
#include <SOURCE/spellTypes.h>
#include <SOURCE/terrainTypes.h>

// forward declarations:
class army;
class armyGroup;
class hero;
class heroWindow;
class icon;
class town;
struct tag_message;

// Combat commands: GetCommand derives one from the hovered hex
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

// The queued gNextAction that the combat loop
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
// pointing from that side.
H1_ENUM_ID_BEGIN(CombatPointerCode)
COMBAT_POINTER_VIEW = 5, COMBAT_POINTER_DEFAULT = 6,
                         COMBAT_POINTER_ATTACK_FIRST = 7 H1_ENUM_ID_END(CombatPointerCode)

                         // Two sides (defender 0, attacker 1) index m_armies and m_numArmies.
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
    // army::m_targetIndex without a target.
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

// Per-side draw sentinels: m_heroClass and m_catapultFrame hold -1 for a side
// without a hero or catapult (DrawFrame skips the tent / catapult);
// ResetLimitCreature marks a dead stack's m_limitCreatureCount HIDDEN so
// DrawFrame never grows the redraw box for it.
H1_ENUM_CONST_BEGIN(CombatDrawStateConstant)
    COMBAT_HERO_CLASS_NONE = -1,
    COMBAT_CATAPULT_FRAME_NONE = -1,
    COMBAT_LIMIT_CREATURE_HIDDEN = -1,
    // m_wallDamage without damage frames to draw (hexcell::DrawWall).
    COMBAT_WALL_DAMAGE_NONE = -1,
    // m_wallFrame while no wall animation runs (CatAttack resets it).
    COMBAT_WALL_FRAME_NONE = -1
H1_ENUM_CONST_END(CombatDrawStateConstant)

// Combat AI tuning thresholds: AICheckRetreat's artifact-value and army-strength
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

// DoCompAI's plan for the acting stack: shooters with shots left shoot,
// flyers fly, the rest walk.
H1_ENUM_BEGIN(CombatAIAttackPlan)
    COMBAT_AI_ATTACK_NONE = 0,
    COMBAT_AI_ATTACK_SHOOT = 1,
    COMBAT_AI_ATTACK_FLY = 2,
    COMBAT_AI_ATTACK_WALK = 3
H1_ENUM_END(CombatAIAttackPlan)

// Combat manager, 0x7d3 bytes (InitMainClasses; constructor 0x0044b440).
// GameUnsaved reads the baseManager m_active word through gCombatManager.
#pragma pack(push, 1)
class combatManager : public baseManager {
public:
    bool m_restoreMusicSuspension;
    bool m_restoreSampleSuspension;
    H1_ENUM_STORAGE(MusicTrack, i32) m_savedMusicTrack;
    // Open loads kb.pal here and fades the screen in with it.
    class palette* m_combatPalette;
    hexcell m_hexCells[COMBAT_HEX_COUNT];
    // First grid row (0-4) UpdateCombatArea must redraw; 5 when clean.
    i16 m_gridUpdateRow;
    // DrawFrame draws each row's occupants right to left while this is set
    // (SetDrawRightToLeft).
    i8 m_drawRightToLeft;
    // LoadIcons indexes the ground and obstacle tables by this terrain;
    // GetBackgroundName forces 6 for a graveyard field.
    H1_ENUM_STORAGE(TerrainType, i8) m_terrainType;
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
    H1_ENUM_ARRAY(class icon*, m_combatIcons, CombatIconSlot, COMBAT_ICON_COUNT);
    // Clean combat background: FlyTo and army::Walk restore the screen from it.
    class bitmap* m_backgroundBuffer;
    i8 m_backgroundDrawn;
    // GetBackgroundName reads the trigger of the cell the battle is on.
    class mapCell* m_battlefieldCell;
    // Per side: the town fought in. DoVictory gives the defender's winner
    // the castle bonus; AICheckRetreat never retreats from a town.
    H1_ENUM_ARRAY(class town*, m_combatTowns, CombatSide, COMBAT_SIDE_COUNT);
    H1_ENUM_ARRAY(class hero*, m_heroes, CombatSide, COMBAT_SIDE_COUNT);
    // Per side: the army group fought with (ViewArmy hands it to
    // game::ViewArmy).
    H1_ENUM_ARRAY(class armyGroup*, m_armyGroups, CombatSide, COMBAT_SIDE_COUNT);
    // Set by a surrender (ProcessNextAction).
    H1_ENUM_ARRAY(i8, m_sideSurrendered, CombatSide, COMBAT_SIDE_COUNT);
    // SetupCombat copies gHumanPlayer per side; a bad-morale roll may spare
    // a computer side.
    H1_ENUM_ARRAY(char, m_humanPlayerSide, CombatSide, COMBAT_SIDE_COUNT);
    H1_ENUM_ARRAY(i8, m_playerId, CombatSide, COMBAT_SIDE_COUNT);
    H1_ENUM_ARRAY(i32, m_experienceValue, CombatSide, COMBAT_SIDE_COUNT);
    H1_ENUM_ARRAY(i8, m_heroCastSpell, CombatSide, COMBAT_SIDE_COUNT);
    // Live stacks per side (CastMassSpell walks each side's armies).
    H1_ENUM_ARRAY(i16, m_numArmies, CombatSide, COMBAT_SIDE_COUNT);
    H1_ENUM_ARRAY_ROWS(army, m_armies, CombatSide, COMBAT_SIDE_COUNT, COMBAT_SIDE_ARMY_COUNT);
    H1_ENUM_STORAGE(CombatSide, i8) m_currentSide;
    i8 m_currentArmyIndex;
    H1_ENUM_STORAGE(CreatureSpeed, i8) m_currentSpeed;
    i8 m_gridSelectionDisabled;
    i8 m_limitCreature;
    i8 m_limitCreatureHex;
    // army::DrawToBuffer draws the quantity box.
    i8 m_showArmyQuantities;
    i8 m_selectedHex;
    i8 m_directionTargetHex;
    H1_ENUM_STORAGE(CombatMessageCommand, i8) m_previousCommand;
    H1_ENUM_STORAGE(CombatMessageCommand, i8) m_currentCommand;
    // CatAttack animates the side's catapult through these frames.
    H1_ENUM_ARRAY(i16, m_catapultFrame, CombatSide, COMBAT_SIDE_COUNT);
    H1_ENUM_ARRAY(i16, m_catapultAttackCount, CombatSide, COMBAT_SIDE_COUNT);
    H1_ENUM_ARRAY(i16, m_catapultAttacksRemaining, CombatSide, COMBAT_SIDE_COUNT);
    H1_ENUM_ARRAY(i16, m_keepAttacksRemaining, CombatSide, COMBAT_SIDE_COUNT);
    // SetupCombat copies each hero's class (-1 without a hero); DrawFrame
    // draws it as the tent frame.
    H1_ENUM_ARRAY(i16, m_heroClass, CombatSide, COMBAT_SIDE_COUNT);
    i16 m_unknown6d9;
    i16 m_unknown6db;
    // Per side: the side fights from a castle. hexcell::DrawTower/DrawWall
    // mirror the castle art from side 1's flag.
    H1_ENUM_ARRAY(i8, m_castleSide, CombatSide, COMBAT_SIDE_COUNT);
    // SetupCombat sets side 0 when the defending town has a garrisoned hero.
    H1_ENUM_ARRAY(char, m_visitingHeroPresent, CombatSide, COMBAT_SIDE_COUNT);
    // CatAttack's target row in the castle wall column.
    i16 m_catapultTargetRow;
    // CatAttack: 1 when the shot only damages the wall, 0 when it falls;
    // hexcell::DrawObstacle keeps the tower during the impact frames.
    i8 m_wallSurvives;
    i16 m_wallFrame;
    i16 m_wallDamage;
    i8 m_unknown6e8;
    // Per side: creatures the attacking ghosts (CREATURE_GHOST) killed; the
    // ghost stack grows by it after the strike. army::DoAttack stores and
    // reloads it with word moves indexed by side.
    H1_ENUM_ARRAY(i16, m_ghostKills, CombatSide, COMBAT_SIDE_COUNT);
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
    H1_ENUM_ARRAY(i8, m_sideRetreated, CombatSide, COMBAT_SIDE_COUNT);
    // Per stack draw state: ResetLimitCreature clears it (-1 for the dead)
    // and army::SpellEffect marks the stack it animates.
    H1_ENUM_ARRAY_ROWS(i32, m_limitCreatureCount, CombatSide, COMBAT_SIDE_COUNT, 5);
    // DrawFrame's extent modes: the first limits the redraw to the boxes of
    // stacks in m_limitCreatureCount, the second restores only the current
    // extent from the background buffer.
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
    // The attack direction of each pointer sector, encoded: COMBAT_DIRECTION_*
    // values, raised by COMBAT_POINTER_SECTOR_FILLED while a gap is filled.
    i8 m_directionMap[24];
    H1_ENUM_STORAGE(CombatHexDirection, i8) m_mouseDirection;
    i8 m_validDirectionCount;
    class heroWindow* m_winLoseWindow;
    H1_ENUM_STORAGE(SpellType, i8) m_selectedSpell;
    // advManager::DoCombat returns and hands on this outcome byte.
    H1_ENUM_STORAGE(CombatSide, i8) m_combatResult;
    // --- constructors ---
    combatManager(void);
    // --- virtual methods (vtable order) ---
    virtual H1_ENUM_RETURN(BaseManagerStatus, i16) Open(i16 priority) OVERRIDE;
    virtual void Close(void) OVERRIDE;
    virtual H1_ENUM_RETURN(MessageDispatchResult, i16) Main(struct tag_message& message) OVERRIDE;
    // --- methods ---
    void NoShowCombatLog(char* message);
    void CombatMessage(char* text, i32 updateScreen);
    void CombatMessage(H1_ENUM_PARAM(CombatMessageCommand, i16) messageType);
    void ResetLimitCreature(void);
    void UpdateCombatArea(void);
    // The upward directions also redraw the row above.
    void
    UpdateGridForMove(i16 hex, H1_ENUM_PARAM(CombatHexDirection, i8) direction, i16 attributes);
    void UpdateGrid(i16 hex, i16 attributes);
    void DrawBackground(void);
    void DrawFrame(i8 updateScreen);
    void SetDrawRightToLeft(i8 rightToLeft);
    i8 ViewGeneral(H1_ENUM_PARAM(CombatSide, i32) side, i32 allowActions, i32 quickView);
    void ViewArmy(class army* viewedArmy, H1_ENUM_PARAM(CombatSide, i32) side, i32 quickView);
    i8 ViewSpells(i32);
    i8 ValidSpellTarget(H1_ENUM_PARAM(SpellType, i8) spell, i8 hex);
    void SpellMessage(H1_ENUM_PARAM(SpellType, i8) spell, i8 hex);
    void
    CastSpell(H1_ENUM_PARAM(SpellType, i8) spell, i8 targetHex, i8 castByCreature, i8 teleportDest);
    void DefaultSpell(i8 targetHex);
    // Cure (one side) and Dispel (both sides) animation; side 2 means both.
    void CastMassSpell(H1_ENUM_PARAM(CombatSide, i8) castSide, i8 cureOnly);
    // Cancels the side's spells after the mass animation.
    void CancelSideSpells(H1_ENUM_PARAM(CombatSide, i8) side, i8 cureOnly);
    void Fireball(i8 targetHex);
    void MeteorShower(i8 targetHex);
    void ElementalStorm(void);
    void Armageddon(void);
    i8 ValidHexToStandOn(i32 hex);
    void SetCombatDirections(i32 targetHex);
    void CheckSetMouseDirection(i32 mouseX, i32 mouseY, i32 targetHex);
    H1_ENUM_RETURN(CombatPointerCode, i32) GetPointer(H1_ENUM_PARAM(CombatMessageCommand, i32) command);
    H1_ENUM_RETURN(MessageDispatchResult, i32) ProcessCombatMsg(struct tag_message& message);
    void ResetRound(void);
    i32 CheckWin(struct tag_message* message);
    H1_ENUM_RETURN(CombatMessageCommand, i8) GetCommand(i16 hex);
    i8 RightClick(i8 hex);
    void DoCommand(H1_ENUM_PARAM(CombatMessageCommand, i8) command);
    void ClearWinLoseBottom(class heroWindow* window);
    void ShowWinLoseArtifact(class heroWindow* window, H1_ENUM_PARAM(ArtifactType, i32) artifact);
    void ShowDeadArmies(class heroWindow* window);
    void DoVictory(H1_ENUM_PARAM(CombatSide, i8) winningSide);
    void DoLoseWindow(void);
    i16 DoSurrender(void);
    void CheckChangeSelector(void);
    void CheckCastleAttack(void);
    void CheckGetAIMove(void);
    void GetControl(void);
    void ResetMouse(void);
    H1_ENUM_RETURN(MessageDispatchResult, i16) ProcessNextAction(struct tag_message& message);
    i32 DoSpellAI(H1_ENUM_PARAM(CombatSide, i8) side);
    void DetermineEffectOfSpell(H1_ENUM_PARAM(SpellType, i32) spell, i32* bestEffect, i32* bestHex);
    i32 EffectSpellCreateCreature(i32 hex, i32 spell);
    i32 RawEffectSpellInfluence(class army* target, H1_ENUM_PARAM(SpellType, i32) spell);
    void ClearEffects(void);
    void NextPos(i32* hex);
    i32 FirstArmy(i32 startHex, H1_ENUM_PARAM(CombatSide, i32) side, i32* hex);
    i32 FirstResurrectable(i32 startHex, i32* hex, i32 spell);
    // DetermineEffectOfSpell passes the effect, then a side and flag, a hex,
    // or the spell, base damage and hex.
    void EffectSpellCure(i32* effect, H1_ENUM_PARAM(CombatSide, i32) targetSide, i8 cureOnly);
    void EffectSpellResurrect(i32* effect, i32 hex);
    void EffectSpellDamage(
        i32* effect,
        H1_ENUM_PARAM(SpellType, i32) spell,
        i32 damagePerPower,
        i32 targetHex
    );
    void CombineGroups(class armyGroup* sourceGroup, class armyGroup* targetGroup);
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
    void UpdateArmyGroup(H1_ENUM_PARAM(CombatSide, i8) side);
    void GenerateMap(void);
    char* GetBackgroundName(void);
    i8 MoreTreesNear(void);
    // No callers; rebuilds the field and redraws.
    void RegenerateField(void);
    void LoadIcons(void);
    void FreeIcons(void);
    void LoadArmies(void);
    void FreeArmies(void);
    i16 GetGridIndex(i16 x, i16 y);
    void CheckApplyGoodMorale(H1_ENUM_PARAM(CombatSide, i32) side, i32 index);
    i32 CheckApplyBadMorale(H1_ENUM_PARAM(CombatSide, i32) side, i32 index);
    i8 GetNextArmy(i32 checkMorale);
    i8 IsWinner(H1_ENUM_PARAM(CombatSide, i8) side);
    void CatAttack(H1_ENUM_PARAM(CombatSide, i8) side);
    // A town has a single keep.
    void KeepAttack(void);
    i32 ExperienceValueOfStack(H1_ENUM_PARAM(CombatSide, i8) side);
    void ResetHitByCreature(void);
    void SaveCombatBorder(void);
    void DrawCombatBorder(void);
    i32 AICheckRetreat(void);
    void DoCompAI(H1_ENUM_PARAM(CombatSide, i8) side);
    i16 GetShooterMask(H1_ENUM_PARAM(CombatSide, i8) side);
    i16 GetFlyerMask(H1_ENUM_PARAM(CombatSide, i8) side);
    i16 GetWalkerMask(H1_ENUM_PARAM(CombatSide, i8) side);
    i16 GetBestArmy(H1_ENUM_PARAM(CombatSide, i8) side, i16 mask);
    i16 GetWorstArmy(H1_ENUM_PARAM(CombatSide, i8) side, i16 mask);
    i16 GetClosestArmy(class army* currentArmy, H1_ENUM_PARAM(CombatSide, i8) side, i16 mask);
    u32 GetStrength(H1_ENUM_PARAM(CombatSide, i8) side, i16 mask);
    i8 AttemptAttack(class army* currentArmy, H1_ENUM_PARAM(CombatSide, i8) side, i16 mask);
    i8 AttemptAdjacentAttack(class army* currentArmy);
    i8 WalkTowardArmyFront(class army* currentArmy, H1_ENUM_PARAM(CombatSide, i8) side, i16 mask);
    i8 WalkTowardArmy(class army* currentArmy, H1_ENUM_PARAM(CombatSide, i8) side, i16 mask);
};
#pragma pack(pop)

i32 ValidHex(i32 hex);
H1_ENUM_RETURN(MessageDispatchResult, i16) WinCombatHandler(struct tag_message& message);
H1_ENUM_RETURN(MessageDispatchResult, i16) CombatSpecialHandler(struct tag_message& message);
H1_ENUM_RETURN(MessageDispatchResult, i16) HandleCastSpell(struct tag_message& message);
// HandleCastSpell: the hex under the spell pointer (0x004906b4) and the
// teleport second-click state (0x00490690).
extern i8 gInTeleportGetDest;
// Frame of the mass-spell glow drawn by DrawFrame (0x004c78b4).
extern i16 gCombatFxFrame;
// Captured artifacts shown page by page on the victory window.
#define gMaxTransferArtifacts iMaxTransferArtifacts // spelling fixes .bss order
extern i8 gMaxTransferArtifacts;
#define gCurTransferArtifact iCurTransferArtifact // spelling fixes .bss order
extern i32 gCurTransferArtifact;
// DoSurrender: gold the enemy hero asks for (0x004a4bac).
#define gSurrenderCost giSurrenderCost // spelling fixes .bss order
extern i32 gSurrenderCost;
// The queued combat action and its grid/extra arguments (0x004a4bc0..).
extern H1_ENUM_STORAGE(CombatAction, i32) gNextAction;
#define gNextActionGridIndex giNextActionGridIndex // spelling fixes .bss order
extern i32 gNextActionGridIndex;
#define gNextActionExtra giNextActionExtra // spelling fixes .bss order
extern i32 gNextActionExtra;
#define gNextActionGridIndex2 giNextActionGridIndex2 // spelling fixes .bss order
extern i32 gNextActionGridIndex2;
// Queue a move (or attack) toward a hex: the action is stored before the
// hex expression is evaluated; other action fields stay with the caller.
#define SET_NEXT_COMBAT_MOVE(hex) (gNextAction = ACTION_MOVE, gNextActionGridIndex = (hex))
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
extern H1_ENUM_ARRAY(char*, gCombatMessage, CombatMessageText, COMBAT_TEXT_COUNT);
// Fallback net player for a combat action broadcast (0x004c6710).
extern i32 gRemoteDefaultPlayer;
#define gTransferArtifacts iTransferArtifacts // spelling fixes .bss order
extern H1_ENUM_STORAGE(ArtifactType, i8) gTransferArtifacts[];
// Network combat: this machine controls the current side (0x004a4b98).
#define gThisNetHasControl gbThisNetHasControl // spelling fixes .bss order
extern i8 gThisNetHasControl;
// gCombatHelp rows ProcessCombatMsg shows when the pointer is off the grid:
// over the auto-combat strip (left), the skip strip (right), or neither.
H1_ENUM_BEGIN(CombatHelpText)
    COMBAT_HELP_AUTO_COMBAT = 0,
    COMBAT_HELP_SKIP_UNIT = 1,
    COMBAT_HELP_NONE = 2,
    COMBAT_HELP_COUNT = 3
H1_ENUM_END(CombatHelpText)
extern H1_ENUM_ARRAY(char*, gCombatHelp, CombatHelpText, COMBAT_HELP_COUNT);

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
    BATTLE_RESULT_EXPERIENCE_AND_LEVELS = 10,
    BATTLE_RESULT_COUNT = 11
H1_ENUM_END(BattleResultText)
// Victory/defeat window texts (0x00493c60).
extern H1_ENUM_ARRAY(char*, gBattleResults, BattleResultText, BATTLE_RESULT_COUNT);

// win/losecmbt.bin widget ids: the animation, the result text, and the
// bottom panel ShowWinLoseArtifact (captured artifact) or ShowDeadArmies
// (casualties: icon/count ids are FIRST + side * ARMY_GROUP_SLOT_COUNT + slot,
// with the count id doubling as the side's "None" line) fills in.
H1_ENUM_ID_BEGIN(CombatWinLoseControl)
WIN_LOSE_ANIMATION = 1, WIN_LOSE_RESULT_TEXT = 0x65, WIN_LOSE_CASUALTY_ICON_FIRST = 0x7d0,
                        WIN_LOSE_ARTIFACT_BACKGROUND = 0x7d1, WIN_LOSE_ARTIFACT_ICON = 0x7d2,
                        WIN_LOSE_CASUALTY_TEXT_FIRST = 0x834, WIN_LOSE_ARTIFACT_NAME = 0x835,
                        WIN_LOSE_CASUALTY_HEADING = 0x83e H1_ENUM_ID_END(CombatWinLoseControl)

                        // m_winLoseBottomTextWidgets slots: side * ARMY_GROUP_SLOT_COUNT + slot for
                        // the casualty counts, then the two side headings and the casualty title.
                        // DoVictory's experience line buffer.
                        H1_ENUM_CONST_BEGIN(CombatVictoryConstant)
    COMBAT_VICTORY_EXPERIENCE_TEXT_SIZE = 152
H1_ENUM_CONST_END(CombatVictoryConstant)

H1_ENUM_CONST_BEGIN(CombatWinLoseSlot)
    WIN_LOSE_SLOT_SIDE_HEADING_FIRST = 10,
    WIN_LOSE_SLOT_CASUALTY_TITLE = 12,
    WIN_LOSE_SLOT_COUNT = 15
H1_ENUM_CONST_END(CombatWinLoseSlot)

// surrendr.bin widget ids DoSurrender fills (the victor's portrait, the offer).
H1_ENUM_ID_BEGIN(SurrenderControl)
SURRENDER_PORTRAIT = 1, SURRENDER_TEXT = 2 H1_ENUM_ID_END(SurrenderControl)

                        // SetCombatDirections' rear hex for a one-hex stack: no rear hex to check
                        // (ValidHexToStandOn accepts it; CheckSetMouseDirection's backHex default).
                        H1_ENUM_CONST_BEGIN(CombatRearHexConstant)
    COMBAT_REAR_HEX_UNUSED = -2
H1_ENUM_CONST_END(CombatRearHexConstant)

// Combat-window widget ids ProcessCombatMsg handles: the battlefield (0x40;
// ResetMouse hovers it), the button
// that stops grid selection and hides the pointer, and the skip-turn button
// that queues ACTION_SKIP_TURN.
H1_ENUM_ID_BEGIN(CombatControlId)
COMBAT_CONTROL_DISABLE_SELECTION = 2,
    COMBAT_CONTROL_SKIP_TURN = 8,
    COMBAT_CONTROL_FIELD = 0x40 H1_ENUM_ID_END(CombatControlId)

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

// spelmous.mse frames: HandleCastSpell shows the selected SpellType's own
// frame over a valid target and frame 19, after the combat spells, otherwise.
H1_ENUM_ID_BEGIN(SpellPointerFrame)
    SPELL_POINTER_NO_TARGET = 19
H1_ENUM_ID_END(SpellPointerFrame)

// DetermineEffectOfSpell's target walk: one global evaluation, every area
// position, or each friendly / enemy stack.
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
// attacker's catapult arm shows its released image at RELEASE_FRAME.
H1_ENUM_CONST_BEGIN(CombatDrawFrameConstant)
    COMBAT_WALL_COLLAPSE_FRAME = 5,
    COMBAT_CATAPULT_RELEASE_FRAME = 7
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

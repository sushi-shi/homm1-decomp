#ifndef HOMM1_SOURCE_COMBATMANAGER_H
#define HOMM1_SOURCE_COMBATMANAGER_H

#include <BASE/baseManager.h>
#include <SOURCE/army.h>
#include <SOURCE/hexcell.h>
#include <SOURCE/spellTypes.h>
#include <SOURCE/terrainTypes.h>

class army;
class armyGroup;
class hero;
class heroWindow;
class icon;
class town;
struct SBolt;
struct tag_message;

enum CombatMessageCommand {
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
};

enum CombatAction {
    ACTION_NONE = 0,
    ACTION_CAST_SPELL = 1,
    ACTION_MOVE = 2,
    ACTION_SKIP_TURN = 3,
    ACTION_RETREAT = 4,
    ACTION_SURRENDER = 5,
    ACTION_ATTACK = 6
};

enum CombatPointerCode {
    COMBAT_POINTER_VIEW = 5,
    COMBAT_POINTER_DEFAULT = 6,
    COMBAT_POINTER_ATTACK_FIRST = 7
};

enum CombatGridConstant {
    COMBAT_HEX_COUNT = 45,
    COMBAT_SIDE_ARMY_COUNT = 6,
    COMBAT_CASTLE_WALL_COLUMN = 5,
    COMBAT_ARMY_INDEX_NONE = -1
};

enum CombatViewConstant {
    COMBAT_VIEW_HEIGHT = 460,
    COMBAT_EXTENT_MIN_START = 640,
    COMBAT_HEX_WIDTH = 78,
    COMBAT_HEX_HEIGHT = 80,
    COMBAT_FIELD_TOP = 60,
    COMBAT_HEX_ORIGIN_Y = 139,
    COMBAT_ATTACKER_HERO_ROW = 1,
    COMBAT_DEFENDER_HERO_ROW = 2,
    COMBAT_CATAPULT_ROW = 3
};

enum CombatDrawStateConstant {
    COMBAT_HERO_TYPE_NONE = -1,
    COMBAT_CATAPULT_FRAME_NONE = -1,
    COMBAT_LIMIT_CREATURE_HIDDEN = -1,
    COMBAT_WALL_DAMAGE_NONE = -1,
    COMBAT_WALL_FRAME_NONE = -1
};

enum CombatAIConstant {
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
    COMBAT_AI_CASTLE_BASE_ARCHERS = 5,
    COMBAT_AI_CASTLE_ARCHERS_PER_DWELLING = 4,
    COMBAT_AI_CASTLE_ARCHER_STRENGTH = 100
};

enum CombatAIAttackPlan {
    COMBAT_AI_ATTACK_NONE = 0,
    COMBAT_AI_ATTACK_SHOOT = 1,
    COMBAT_AI_ATTACK_FLY = 2,
    COMBAT_AI_ATTACK_WALK = 3
};

enum CombatIconSlot {
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
};

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

#pragma pack(push, 1)
class combatManager : public baseManager {
public:
    char m_unknown30[0xc];
    class palette* m_combatPalette;
    hexcell m_hexCells[COMBAT_HEX_COUNT];
    i16 m_gridUpdateRow;
    i8 m_gridMode;
    i8 m_terrainType;
    i8 m_extendLimitDown;
    class town* m_originalCombatTown;
    class font* m_smallFont;
    char m_unknown269[4];
    char* m_savedBorder;
    class icon* m_combatIcons[COMBAT_ICON_COUNT];
    class bitmap* m_backgroundBuffer;
    i8 m_backgroundDrawn;
    class mapCell* m_battlefieldCell;
    class town* m_combatTowns[2];
    class hero* m_heroes[2];
    class armyGroup* m_armyGroups[2];
    i8 m_sideDefeated[2];
    char m_humanSide[2];
    i8 m_playerId[2];
    i32 m_experienceValue[2];
    i8 m_heroCastSpell[2];
    i16 m_numArmies[COMBAT_SIDE_COUNT];
    army m_armies[COMBAT_SIDE_COUNT][COMBAT_SIDE_ARMY_COUNT];
    i8 m_currentSide;
    i8 m_currentArmyIndex;
    i8 m_currentSpeed;
    i8 m_gridSelectionDisabled;
    i8 m_limitCreature;
    i8 m_limitCreatureHex;
    i8 m_showArmyQuantities;
    i8 m_selectedHex;
    i8 m_directionTargetHex;
    i8 m_previousCommand;
    i8 m_currentCommand;
    i16 m_catapultFrame[2];
    i16 m_catapultAttackCount[2];
    i16 m_catapultAttacksRemaining[2];
    i16 m_keepAttacksRemaining[2];
    i16 m_heroType[2];
    i16 m_unknown6d9;
    i16 m_unknown6db;
    i8 m_castleSide[2];
    char m_visitingHeroPresent[2];
    i16 m_catapultTarget;
    i8 m_wallSurvives;
    i16 m_wallFrame;
    i16 m_wallDamage;
    i8 m_unknown6e8;
    i16 m_ghostKills[2];
    class bitmap* m_backgroundBitmap;
    class heroWindow* m_combatWindow;
    char m_unknown6f5[4];
    i16 m_unknown6f9;
    i16 m_messageTypeMask;
    i8 m_sideRetreated[2];
    i32 m_limitCreatureCount[2][5];
    i32 m_computeExtent;
    i32 m_redrawExtent;
    i32 m_combatWindowOpen;
    class widget* m_winLoseBottomWidgets[15];
    class widget* m_winLoseBottomTextWidgets[15];
    i32 m_combatX;
    i32 m_combatY;
    i8 m_directionMap[24];
    i8 m_mouseDirection;
    i8 m_validDirectionCount;
    class heroWindow* m_winLoseWindow;
    i8 m_selectedSpell;
    i8 m_combatResult;
    combatManager(void);
    virtual i16 Open(i16 priority) ;
    virtual void Close(void) ;
    virtual i16 Main(struct tag_message& message) ;
    void NoShowCombatLog(char*);
    void ClearCombatMessages(i32 force);
    void CheckUpdateCombatMessages(void);
    void CombatMessage(char* text, i32 updateScreen);
    void CombatMessage(i16 messageType);
    void ResetLimitCreature(void);
    void UpdateCombatArea(void);
    void SetupGridForArmy(class army* armyPointer);
    void UpdateGridForMove(i16 hex, i8 direction, i32 attributes);
    void UpdateGrid(i16 hex, i32);
    void DrawBackground(void);
    void UpdateMouseGrid(i32 hexIndex, i32 forceUpdate);
    void DrawFrame(i8 updateScreen);
    void SetGridMode(i8 mode);
    void DrawSmallView(i32 viewIndex, i32 updateScreen);
    i8 ViewGeneral(i32 side, i32 allowActions, i32 quickView);
    void ViewArmy(class army* viewedArmy, i32 side, i32 quickView);
    i32 HasValidSpellTarget(i32 spell);
    i8 ViewSpells(i32);
    i32 FindResurrectArmyIndex(i32 side, i32 spell, i32 hex);
    i8 ValidSpellTarget(i8 spell, i8 hex);
    void SpellMessage(i8 spell, i8 hex);
    void
    CastSpell(i8 spell, i8 targetHex, i8 castByCreature, i8 teleportDest);
    void DefaultSpell(i8 targetHex);
    void CastMassSpell(i8 castSide, i8 cureOnly);
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
    i32 GetPointer(i32 command);
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
    i32 DoSpellAI(i8 side);
    void DetermineEffectOfSpell(i32 spell, i32* bestEffect, i32* bestHex);
    i32 EffectSpellCreateCreature(i32 hex, i32 spell);
    i32 RawEffectSpellInfluence(class army* target, i32 spell);
    void ClearEffects(void);
    void NextPos(i32* hex);
    i32 FirstArmy(i32 startHex, i32 side, i32* hex);
    i32 FirstResurrectable(i32 startHex, i32* hex, i32 spell);
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
    void UpdateArmyGroup(i8 side);
    void GenerateMap(void);
    char* GetBackgroundName(void);
    i8 MoreTreesNear(void);
    void RegenerateField(void);
    void LoadIcons(void);
    void FreeIcons(void);
    void LoadArmies(void);
    void FreeArmies(void);
    i16 GetGridIndex(i16 x, i16 y);
    void CheckApplyGoodMorale(i32 side, i32 index);
    i32 CheckApplyBadMorale(i32 side, i32 index);
    i8 GetNextArmy(i32 checkMorale);
    i8 IsWinner(i8 side);
    void CatAttack(i8 side);
    void KeepAttack(void);
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
    void DoCompAI(i8);
    float GetModLichDamage(class army* target, float damage);
    void DoLichShot(class army* lich);
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
extern i8 gInTeleportGetDest;
extern i16 giCombatFxFrame;
extern i8 iMaxTransferArtifacts;
extern i32 iCurTransferArtifact;
extern i32 giSurrenderCost;
extern i32 giNextAction;
extern i32 giNextActionGridIndex;
extern i32 giNextActionExtra;
extern i32 giNextActionGridIndex2;
enum CombatMessageText {
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
};
extern i32 giRemoteDefaultPlayer;
extern i8 iTransferArtifacts[];
extern i8 gbThisNetHasControl;
#endif

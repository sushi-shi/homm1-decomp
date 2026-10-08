#ifndef HOMM1_SOURCE_COMBATMANAGER_H
#define HOMM1_SOURCE_COMBATMANAGER_H

#include <BASE/audioTypes.h>
#include <BASE/baseManager.h>
#include <BASE/message.h>
#include <SOURCE/army.h>
#include <SOURCE/artifactTypes.h>
#include <SOURCE/hexcell.h>
#include <SOURCE/REMOTE.h>
#include <SOURCE/spellTypes.h>
#include <SOURCE/terrainTypes.h>

class army;
class armyGroup;
class hero;
class heroWindow;
class icon;
class town;
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
    COMBAT_HERO_CLASS_NONE = -1,
    COMBAT_CATAPULT_FRAME_NONE = -1,
    COMBAT_CATAPULT_FRAME_FIRST = 0,
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

enum CombatNearbyObjectConstant {
    COMBAT_NEARBY_MOUNTAIN = 0,
    COMBAT_NEARBY_TREE = 1
};

enum CombatTurnConstant {
    COMBAT_SPEED_PASS_COUNT = 5
};

enum CombatAIAttackPlan {
    COMBAT_AI_ATTACK_NONE = 0,
    COMBAT_AI_ATTACK_SHOOT = 1,
    COMBAT_AI_ATTACK_FLY = 2,
    COMBAT_AI_ATTACK_WALK = 3
};

#pragma pack(push, 1)
class combatManager : public baseManager {
public:
    bool m_restoreMusicSuspension;
    bool m_restoreSampleSuspension;
    i32 m_savedMusicTrack;
    class palette* m_combatPalette;
    hexcell m_hexCells[COMBAT_HEX_COUNT];
    i16 m_gridUpdateRow;
    i8 m_drawRightToLeft;
    i8 m_terrainType;
    b8 m_extendLimitDown;
    class town* m_originalCombatTown;
    class font* m_smallFont;
    char* m_savedBorder;
    class icon* m_combatIcons[COMBAT_ICON_COUNT];
    class bitmap* m_backgroundBuffer;
    b8 m_backgroundDrawn;
    class mapCell* m_battlefieldCell;
    class town* m_combatTowns[COMBAT_SIDE_COUNT];
    class hero* m_heroes[COMBAT_SIDE_COUNT];
    class armyGroup* m_armyGroups[COMBAT_SIDE_COUNT];
    i8 m_sideSurrendered[COMBAT_SIDE_COUNT];
    char m_humanPlayerSide[COMBAT_SIDE_COUNT];
    i8 m_playerId[COMBAT_SIDE_COUNT];
    i32 m_experienceValue[COMBAT_SIDE_COUNT];
    i8 m_heroCastSpell[COMBAT_SIDE_COUNT];
    i16 m_numArmies[COMBAT_SIDE_COUNT];
    army m_armies[COMBAT_SIDE_COUNT][COMBAT_SIDE_ARMY_COUNT];
    i8 m_currentSide;
    i8 m_currentArmyIndex;
    i8 m_currentSpeed;
    b8 m_autoCombat;
    b8 m_selectorVisible;
    i8 m_selectorHex;
    b8 m_showArmyQuantities;
    i8 m_selectedHex;
    i8 m_directionTargetHex;
    i8 m_previousCommand;
    i8 m_currentCommand;
    i16 m_catapultFrame[COMBAT_SIDE_COUNT];
    i16 m_catapultAttackCount[COMBAT_SIDE_COUNT];
    i16 m_catapultAttacksRemaining[COMBAT_SIDE_COUNT];
    i16 m_keepAttacksRemaining[COMBAT_SIDE_COUNT];
    i16 m_heroClass[COMBAT_SIDE_COUNT];
    i16 m_unused6d9;
    i16 m_unused6db;
    i8 m_castleSide[COMBAT_SIDE_COUNT];
    b8 m_visitingHeroPresent[COMBAT_SIDE_COUNT];
    i16 m_catapultTargetRow;
    b8 m_wallSurvives;
    i16 m_wallFrame;
    i16 m_wallDamage;
    i8 m_unused6e8;
    i16 m_ghostKills[COMBAT_SIDE_COUNT];
    class bitmap* m_backgroundBitmap;
    class heroWindow* m_combatWindow;
    char m_unused6f5[4];
    i16 m_unused6f9;
    i16 m_messageTypeMask;
    i8 m_sideRetreated[COMBAT_SIDE_COUNT];
    i32 m_limitCreatureCount[COMBAT_SIDE_COUNT][ARMY_GROUP_SLOT_COUNT];
    b32 m_computeExtent;
    b32 m_redrawExtent;
    b32 m_combatWindowOpen;
    class widget* m_winLoseBottomWidgets[15];
    class widget* m_winLoseBottomTextWidgets[15];
    i32 m_battleSiteX;
    i32 m_battleSiteY;
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
    void NoShowCombatLog(char* message);
    void CombatMessage(char* text, b32 updateScreen);
    void CombatMessage(i16 messageType);
    void ResetLimitCreature(void);
    void UpdateCombatArea(void);
    void
    UpdateGridForMove(i16 hex, i8 direction, i16 attributes);
    void UpdateGrid(i16 hex, i16 attributes);
    void DrawBackground(void);
    void DrawFrame(b8 updateScreen);
    void SetDrawRightToLeft(i8 rightToLeft);
    i8 ViewGeneral(i32 side, b32 allowActions, b32 quickView);
    void ViewArmy(class army* viewedArmy, i32 side, i32 quickView);
    i8 ViewSpells(i32);
    i8 ValidSpellTarget(i8 spell, i8 hex);
    void SpellMessage(i8 spell, i8 hex);
    void
    CastSpell(i8 spell, i8 targetHex, b8 castByCreature, i8 teleportDest);
    void DefaultSpell(i8 targetHex);
    void CastMassSpell(i8 castSide, i8 cureOnly);
    void CancelSideSpells(i8 side, i8 cureOnly);
    void Fireball(i8 targetHex);
    void MeteorShower(i8 targetHex);
    void ElementalStorm(void);
    void Armageddon(void);
    i8 ValidHexToStandOn(i32 hex);
    void SetCombatDirections(i32 targetHex);
    void CheckSetMouseDirection(i32 mouseX, i32 mouseY, i32 targetHex);
    i32 GetPointer(i32 command);
    i32 ProcessCombatMsg(struct tag_message& message);
    void ResetRound(void);
    i32 CheckWin(struct tag_message* message);
    i8 GetCommand(i16 hex);
    i8 RightClick(i8 hex);
    void DoCommand(i8 command);
    void ClearWinLoseBottom(class heroWindow* window);
    void ShowWinLoseArtifact(class heroWindow* window, i32 artifact);
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
    i32 DoSpellAI(i8 side);
    void DetermineEffectOfSpell(i32 spell, i32* bestEffect, i32* bestHex);
    i32 EffectSpellCreateCreature(i32 hex, i32 spell);
    i32 RawEffectSpellInfluence(class army* target, i32 spell);
    void ClearEffects(void);
    void NextPos(i32* hex);
    b32 FirstArmy(i32 startHex, i32 side, i32* hex);
    i32 FirstResurrectable(i32 startHex, i32* hex, i32 spell);
    void EffectSpellCure(i32* effect, i32 targetSide, b8 cureOnly);
    void EffectSpellResurrect(i32* effect, i32 hex);
    void EffectSpellDamage(
        i32* effect,
        i32 spell,
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
        i32 battleSiteX,
        i32 battleSiteY,
        i32 randomSeed
    );
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
    i8 GetNextArmy(b32 checkMorale);
    i8 IsWinner(i8 side);
    void CatAttack(i8 side);
    void KeepAttack(void);
    i32 ExperienceValueOfStack(i8 side);
    void ResetHitByCreature(void);
    void SaveCombatBorder(void);
    void DrawCombatBorder(void);
    i32 AICheckRetreat(void);
    void DoCompAI(i8 side);
    i16 GetShooterMask(i8 side);
    i16 GetFlyerMask(i8 side);
    i16 GetWalkerMask(i8 side);
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
extern b8 gInTeleportGetDest;
extern i16 gCombatFxFrame;
extern i8 gMaxTransferArtifacts;
extern i32 gCurTransferArtifact;
extern i32 gSurrenderCost;
extern i32 gNextAction;
extern i32 gNextActionGridIndex;
extern i32 gNextActionExtra;
extern i32 gNextActionGridIndex2;
#define SET_NEXT_COMBAT_MOVE(hex) (gNextAction = ACTION_MOVE, gNextActionGridIndex = (hex))
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
extern char* gCombatMessage[COMBAT_TEXT_COUNT];
extern i32 gRemoteDefaultPlayer;
extern i8 gTransferArtifacts[];
extern b8 gThisNetHasControl;
enum CombatHelpText {
    COMBAT_HELP_AUTO_COMBAT = 0,
    COMBAT_HELP_SKIP_UNIT = 1,
    COMBAT_HELP_NONE = 2,
    COMBAT_HELP_COUNT = 3
};
extern char* gCombatHelp[COMBAT_HELP_COUNT];

enum BattleResultText {
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
};
extern char* gBattleResults[BATTLE_RESULT_COUNT];

enum CombatWinLoseControl {
    WIN_LOSE_ANIMATION = 1,
    WIN_LOSE_RESULT_TEXT = 0x65,
    WIN_LOSE_CASUALTY_ICON_FIRST = 0x7d0,
    WIN_LOSE_ARTIFACT_BACKGROUND = 0x7d1,
    WIN_LOSE_ARTIFACT_ICON = 0x7d2,
    WIN_LOSE_CASUALTY_TEXT_FIRST = 0x834,
    WIN_LOSE_ARTIFACT_NAME = 0x835,
    WIN_LOSE_CASUALTY_HEADING = 0x83e
};

enum CombatVictoryConstant {
    COMBAT_VICTORY_EXPERIENCE_TEXT_SIZE = 152
};

enum CombatWinLoseSlot {
    WIN_LOSE_SLOT_SIDE_HEADING_FIRST = 10,
    WIN_LOSE_SLOT_CASUALTY_TITLE = 12,
    WIN_LOSE_SLOT_COUNT = 15
};

enum SurrenderControl {
    SURRENDER_PORTRAIT = 1,
    SURRENDER_TEXT = 2
};

enum CombatRearHexConstant {
    COMBAT_REAR_HEX_UNUSED = -2
};

enum CombatControlId {
    COMBAT_CONTROL_NONE = 0,
    COMBAT_CONTROL_AUTO_COMBAT = 2,
    COMBAT_CONTROL_SKIP_TURN = 8,
    COMBAT_CONTROL_FIELD = 0x40
};

enum CombatStatusLineConstant {
    COMBAT_STATUS_FIRST_CONTROL = 2,
    COMBAT_STATUS_TEXT_CONTROL = 0xc,
    COMBAT_STATUS_X = 0x30,
    COMBAT_STATUS_Y = 0x1cc,
    COMBAT_STATUS_WIDTH = 0x21f,
    COMBAT_STATUS_HEIGHT = 0x14
};

enum CombatBackground {
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
};

enum SpellPointerFrame {
    SPELL_POINTER_NO_TARGET = 19
};

enum CombatSpellAITargetMode {
    SPELL_AI_GLOBAL = 0,
    SPELL_AI_AREA = 1,
    SPELL_AI_FRIENDLY = 2,
    SPELL_AI_ENEMY = 3
};

enum CastleWallRow {
    COMBAT_UPPER_WALL_ROW = 1,
    COMBAT_LOWER_WALL_ROW = 3
};

enum CombatDrawFrameConstant {
    COMBAT_WALL_COLLAPSE_FRAME = 5,
    COMBAT_CATAPULT_RELEASE_FRAME = 7
};

enum CombatCursorSector {
    COMBAT_CURSOR_SECTOR_RIGHT_UP = 0,
    COMBAT_CURSOR_SECTOR_RIGHT_DOWN = 6,
    COMBAT_CURSOR_SECTOR_LEFT_DOWN = 12,
    COMBAT_CURSOR_SECTOR_LEFT_UP = 18
};

#endif

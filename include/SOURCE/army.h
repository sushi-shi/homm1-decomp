#ifndef HOMM1_SOURCE_ARMY_H
#define HOMM1_SOURCE_ARMY_H

#include <SOURCE/combatTypes.h>
#include <SOURCE/creatureTypes.h>
#include <SOURCE/PATH.h>
#include <SOURCE/spellTypes.h>

enum ArmyAnimationSequence {
    ARMY_ANIMATION_STAND = 0,
    ARMY_ANIMATION_WALK = 1,
    ARMY_ANIMATION_ATTACK = 2,
    ARMY_ANIMATION_EFFECT = 3
};

enum ArmySampleType {
    ARMY_SAMPLE_MOVE = 0,
    ARMY_SAMPLE_ATTACK = 1,
    ARMY_SAMPLE_WINCE = 2,
    ARMY_SAMPLE_SHOOT = 3,
    ARMY_SAMPLE_COUNT = 4
};

enum ArmySpellCancelType {
    ARMY_CANCEL_SPELLS_NONE = -1,
    ARMY_CANCEL_SPELLS_FIRST = 0,
    ARMY_CANCEL_SPELLS_AFTER_MOVE = 0,
    ARMY_CANCEL_SPELLS_AFTER_ATTACK = 1,
    ARMY_CANCEL_SPELLS_AFTER_DAMAGE = 2,
    ARMY_CANCEL_SPELLS_ROUNDS_ONLY = 3
};

enum ArmyDamageMode {
    ARMY_DAMAGE_RANDOM = 0,
    ARMY_DAMAGE_MINIMUM = 1,
    ARMY_DAMAGE_HALF = 2,
    ARMY_DAMAGE_MAXIMUM = 3
};

enum ArmyPowConstant {
    ARMY_POW_NONE = -1,
    ARMY_POW_FRAMES_HIT = 4,
    ARMY_POW_FRAMES_KILLED = 5
};

enum ArmyCombatConstant {
    ARMY_QUANTITY_TEXT_SIZE = 12,
    ARMY_PATH_BLOCKED = 3,
    ARMY_CASTLE_WALL_DEFENSE_BONUS = 4,
    // The battlefield count label shows thousands ("2k") from here on.
    ARMY_COUNT_THOUSANDS = 1000,
    ARMY_PROTECTION_DEFENSE_BONUS = 3
};

enum ArmyLuck {
    ARMY_LUCK_BAD = -1,
    ARMY_LUCK_NONE = 0,
    ARMY_LUCK_GOOD = 1
};

#define CLEAR_ARMY_TARGET(a)                                                                       \
    ((a)->m_targetSide = COMBAT_SIDE_NONE, (a)->m_targetIndex = COMBAT_ARMY_INDEX_NONE)

#define ARMY_FACING_ORIENTATION(facing) (facing)

class army {
public:
    i8 m_targetSide;
    i8 m_targetIndex;
    i16 m_attackDirection;
    i8 m_unused04;
    i8 m_moveTargetHex;
    i8 m_creatureType;
    i8 m_hex;
    i8 m_animationSequence;
    i8 m_animationFrame;
    i8 m_facing;
    i16 m_walkYStep;
    i16 m_initialQuantity;
    i16 m_quantity;
    i16 m_hitPointsLost;
    i8 m_damageMode;
    i8 m_baseSpeed;
    i8 m_luck;
    tag_monsterStats m_stats;
    i16 m_unused29;
    i16 m_powFrames;
    i8 m_side;
    i8 m_index;
    i32 m_effectAnimation;
    b8 m_drawShadow;
    b8 m_hitByCreature;
    class icon* m_standIcon;
    class icon* m_walkIcon;
    class icon* m_attackIcon;
    class sample* m_samples[ARMY_SAMPLE_COUNT];
    i8 m_spellEffect;
    i8 m_spellEndCondition;
    i8 m_spellRounds;
    army(void);
    i32 IsAlive(void) {
        return m_creatureType >= CREATURE_FIRST && m_quantity > 0;
    }
    void InitClean(void);
    void Init(
        i8 creatureType,
        i16 quantity,
        i8 side,
        i8 index
    );
    void LoadResources(void);
    void FreeResources(void);
    void DrawToBuffer(i16 x, i16 y);
    void Stand(b8 redraw);
    void Wince(void);
    void Walk(i16 direction, b8 standAfter, b8 continued);
    void SpecialAttack(void);
    void DirDoAttack(i16 direction);
    void DoHydraAttack(void);
    void DoAttack(b32 retaliation);
    void ResetPath(void);
    i16 WalkTo(void);
    i16 WalkTo(i16 destination);
    i16 AttackTo(void);
    i16 AttackTo(i16 destination);
    void CheckLuck(void);
    void DamageEnemy(
        class army* target,
        i32* damageResult,
        i32* killedResult,
        b32 rangedAttack,
        i32 defenseModifier
    );
    i32 Damage(i32 damage);
    void PowEffect(i8 effect);
    u32 Strength(void);
    void SpellEffect(i16 effect, i32 frameDelay);
    void GoBerserk(void);
    void MoveAttack(i32 destination, b32 moveOnly);
    void CancelSpell(void);
    i16 CanFit(i16* hex);
    i16 ValidFlight(i16 destination, i8 pathMode);
    i16 FlyTo(void);
    i16 FlyTo(i16 destination);
    i16 FindPath(
        i16 sourceHex,
        i16 targetHex,
        i8 speed,
        b8 ignoreSpeed,
        i8 pathMode
    );
    i16 ValidPath(i16 targetHex, i8 pathMode);
    i16 GetMoveMask(i16 sourceHex);
    i16 GetAttackMask(i16 sourceHex, i8 targetMode, i8 targetHex);
    i16 ValidMove(i16 direction);
    i16 ValidMove(i16 sourceHex, i16 direction);
    i16 ValidAttack(
        i16 sourceHex,
        i16 direction,
        i16 targetMode,
        i16 requiredTargetHex,
        i16* attackHex
    );
    i16 GetAdjacentCellIndex(i16 hex, i16 direction);
    i16 ValidRange(i16 targetHex);
    i16 GetBestDirection(i16 sourceHex, i16 targetHex, i16 blockedMask);
    b8 ShotCrossesCastleWall(army* target);
    float ScaleDamage(army* target, float total, i32 rangedAttack, i32 defenseModifier);
};

#define ARMY_IGNORES_SPELLS(a)                                                                     \
    ((a)->m_creatureType == CREATURE_DRAGON || (a)->m_spellEffect == SPELL_ANTI_MAGIC)
// Bless and Curse only pin damage to its maximum or minimum, which changes
// nothing for a creature whose damage range is a single value.
#define ARMY_HAS_FIXED_DAMAGE(a) ((a)->m_stats.damageMin == (a)->m_stats.damageMax)

i16 GetAdjacentCellIndexNoArmy(i16 hex, i16 direction);
extern i16 gCurLoadedSpellEffect;
extern b8 gGenieHalf;
enum ArmyDrawingConstant {
    ARMY_LIMIT_OUTLINE_COLOR = 0xe4,
    ARMY_GOOD_SPELL_OUTLINE_COLOR = 0xf7,
    ARMY_BAD_SPELL_OUTLINE_COLOR = 0xe0,
    ARMY_MISSILE_PATCH_WIDTH = 70,
    ARMY_MISSILE_PATCH_HEIGHT = 60,
    ARMY_MISSILE_HALF_WIDTH = 35,
    ARMY_MISSILE_HALF_HEIGHT = 30
};

enum ArmyMessageConstant {
    ARMY_TARGET_NAME_SIZE = 100
};

enum ArmySpecialConstant {
    ARMY_SPECIAL_ROLL_MAX = 5,
    ARMY_SPECIAL_ROLL_HIT = 3,
    ARMY_GENIE_ROLL_HIT = 2,
    ARMY_EDGE_CLIP_FRAME = 2,
    ARMY_EDGE_CLIP_LATER_FRAME = 3
};

#endif

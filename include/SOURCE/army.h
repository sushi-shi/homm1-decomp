#ifndef HOMM1_SOURCE_ARMY_H
#define HOMM1_SOURCE_ARMY_H

#include <SOURCE/combatTypes.h>
#include <SOURCE/creatureTypes.h>
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
    ARMY_SAMPLE_VOLUME = 0x40,
    ARMY_SAMPLE_CHANNEL = 3,
    ARMY_QUANTITY_TEXT_SIZE = 12,
    ARMY_PATH_BLOCKED = 3,
    ARMY_CASTLE_WALL_DEFENSE_BONUS = 4,
    ARMY_PROTECTION_DEFENSE_BONUS = 3
};

enum ArmyLuck {
    ARMY_LUCK_BAD = -1,
    ARMY_LUCK_NONE = 0,
    ARMY_LUCK_GOOD = 1
};

#define CLEAR_ARMY_TARGET(a)                                                                       \
    ((a)->m_targetSide = COMBAT_SIDE_NONE, (a)->m_targetIndex = COMBAT_ARMY_INDEX_NONE)

#pragma pack(push, 1)
class army {
public:
    i8 m_targetSide;
    i8 m_targetIndex;
    i16 m_attackDirection;
    i8 m_unknown04;
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
    i16 m_unknown29;
    i16 m_powFrames;
    i8 m_side;
    i8 m_index;
    i32 m_effectAnimation;
    i8 m_drawShadow;
    i8 m_hitByCreature;
    class icon* m_standIcon;
    class icon* m_walkIcon;
    class icon* m_attackIcon;
    class sample* m_samples[ARMY_SAMPLE_COUNT];
    i8 m_spellEffect;
    i8 m_spellEndCondition;
    i8 m_spellRounds;
    army(void);
    i32 IsAlive(void) {
        return m_creatureType >= 0 && m_quantity > 0;
    }
    void WaitSample(i32);
    void InitClean(void);
    void Init(i8 type, i16 quantity, i8 side, i8 index);
    void LoadResources(void);
    void FreeResources(void);
    void DrawToBuffer(i16 x, i16 y);
    void Stand(i8 redraw);
    void Wince(void);
    void Walk(i16 direction, i8 standAfter, i8 continued);
    void SpecialAttack(void);
    void DirDoAttack(i16 direction);
    void DoHydraAttack(void);
    void DoAttack(i32 retaliation);
    void ResetPath(void);
    i16 WalkTo(void);
    i16 WalkTo(i16 destHex);
    i16 AttackTo(void);
    i16 AttackTo(i16 destHex);
    void CheckLuck(void);
    void DamageEnemy(
        class army* target,
        i32* damageResult,
        i32* killedResult,
        i32 rangedAttack,
        i32 defenseModifier
    );
    i32 Damage(i32 damage);
    void PowEffect(i8 effect);
    u32 Strength(void);
    i32 LeaveNoBody(void);
    void ProcessDeath(i32 immediate);
    void SpellEffect(i16 effect, i32 frameDelay);
    void CancelSpellType(i32 cancelType);
    void CancelIndividualSpell(i32 influence);
    i32 SetSpellInfluence(i32 influence, i32 rounds);
    void DecrementSpellRounds(void);
    void GoBerserk(void);
    void MoveAttack(i32 hex, i32 moveOnly);
    float SpellCastWorkChance(i32 spell);
    i32 SpellCastWorks(i32 spell);
    void DispelGood(void);
    void CancelSpell(void);
    void Cure(i32 amount);
    i32 MidX(void);
    i32 MidY(void);
    i32 TopY(void);
    i32 RightX(void);
    i32 LeftX(void);
    i32 OtherArmyAdjacent(i32 side, i32 index);
    i32 GetPowBaseY(void);
    i16 CanFit(i16* hex);
    i16 ValidFlight(i16 destination, i8 useDestination);
    i16 FlyTo(void);
    i16 FlyTo(i16 destination);
    i16 FindPath(i16 sourceHex, i16 targetHex, i8, i8 ignoreSpeed, i8 pathMode);
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
};
#pragma pack(pop)

i16 GetAdjacentCellIndexNoArmy(i16 hex, i16 direction);
extern i16 gCurLoadedSpellEffect;
extern i8 gGenieHalf;
#endif

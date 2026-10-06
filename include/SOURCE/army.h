#ifndef HOMM1_SOURCE_ARMY_H
#define HOMM1_SOURCE_ARMY_H

#include <Domains.h>
#include <H1/Macros.h>
#include <SOURCE/combatTypes.h>
#include <SOURCE/creatureTypes.h>
#include <SOURCE/spellTypes.h>

// army::m_animationSequence: the pose army::DrawToBuffer draws. 0 stands
// (std icon), 1 walks (wlk icon, Walk and FlyTo), 2 attacks (atk/std
// icon, SpecialAttack), 3 shows a PowEffect/SpellEffect hit.
H1_ENUM_BEGIN(ArmyAnimationSequence)
    ARMY_ANIMATION_STAND = 0,
    ARMY_ANIMATION_WALK = 1,
    ARMY_ANIMATION_ATTACK = 2,
    ARMY_ANIMATION_EFFECT = 3
H1_ENUM_END(ArmyAnimationSequence)

// army::m_samples slots loaded by LoadResources from move%02d, atksnd%02d,
// wince%02d and (shooters only) shoot%02d .82M files.
H1_ENUM_BEGIN(ArmySampleType)
    ARMY_SAMPLE_MOVE = 0,
    ARMY_SAMPLE_ATTACK = 1,
    ARMY_SAMPLE_WINCE = 2,
    ARMY_SAMPLE_SHOOT = 3,
    ARMY_SAMPLE_COUNT = 4
H1_ENUM_END(ArmySampleType)

// army::m_spellEndCondition: what ends m_spellEffect early; Init and
// CancelSpell store NONE.
H1_ENUM_BEGIN(ArmySpellCancelType)
    ARMY_CANCEL_SPELLS_NONE = -1,
    ARMY_CANCEL_SPELLS_AFTER_MOVE = 0,
    ARMY_CANCEL_SPELLS_AFTER_ATTACK = 1,
    ARMY_CANCEL_SPELLS_AFTER_DAMAGE = 2,
    ARMY_CANCEL_SPELLS_ROUNDS_ONLY = 3
H1_ENUM_END(ArmySpellCancelType)

// army::m_damageMode, how DamageEnemy rolls each creature's damage:
// random by default, the minimum or maximum roll (SPELLS' curse/bless
// set them) or a halved total.
H1_ENUM_BEGIN(ArmyDamageMode)
    ARMY_DAMAGE_RANDOM = 0,
    ARMY_DAMAGE_MINIMUM = 1,
    ARMY_DAMAGE_HALF = 2,
    ARMY_DAMAGE_MAXIMUM = 3
H1_ENUM_END(ArmyDamageMode)

// army::m_powFrames: how many PowEffect frames the stack still shows; Damage
// sets the hit and killed counts, Init clears it.
H1_ENUM_CONST_BEGIN(ArmyPowConstant)
    ARMY_POW_NONE = -1,
    ARMY_POW_FRAMES_HIT = 4,
    ARMY_POW_FRAMES_KILLED = 5
H1_ENUM_CONST_END(ArmyPowConstant)

// DrawToBuffer's quantity text buffer and the attack/spell defense modifiers
// (LoadResources plays its samples at sample.h SAMPLE_VOLUME_FULL).
H1_ENUM_CONST_BEGIN(ArmyCombatConstant)
    ARMY_QUANTITY_TEXT_SIZE = 12,
    // WalkTo/AttackTo when no path reaches the target.
    ARMY_PATH_BLOCKED = 3,
    // SpecialAttack: a shot across an intact or damaged castle wall adds
    // this to the target's defense (DamageEnemy's defense modifier).
    ARMY_CASTLE_WALL_DEFENSE_BONUS = 4,
    // Protection's defense bonus; CancelSpell takes it back.
    ARMY_PROTECTION_DEFENSE_BONUS = 3
H1_ENUM_CONST_END(ArmyCombatConstant)

// army::m_luck: CheckLuck rolls good or bad luck for this attack;
// DamageEnemy doubles or halves the damage and resets it.
H1_ENUM_BEGIN(ArmyLuck)
    ARMY_LUCK_BAD = -1,
    ARMY_LUCK_NONE = 0,
    ARMY_LUCK_GOOD = 1
H1_ENUM_END(ArmyLuck)

// Combat stack, 0x54 bytes;
// army::Init copies 0x13 bytes of gMonsterDatabase from +0xc into +0x16.
// Forget an army's attack target; takes a pointer to the army.
#define CLEAR_ARMY_TARGET(a)                                                                       \
    ((a)->m_targetSide = COMBAT_SIDE_NONE, (a)->m_targetIndex = COMBAT_ARMY_INDEX_NONE)

#pragma pack(push, 1)
class army {
public:
    // Attack target (GetCommand clears both to -1).
    i8 m_targetSide;
    i8 m_targetIndex;
    // ValidRange records the chosen attack direction.
    i16 m_attackDirection;
    i8 m_unknown04;
    // ValidPath records the reachable target hex here.
    i8 m_moveTargetHex;
    H1_ENUM_STORAGE(CreatureType, i8) m_creatureType;
    i8 m_hex;
    // DrawToBuffer: pose 0 stand, 1 walk, 2 attack, 3 spell effect and the
    // frame within it.
    H1_ENUM_STORAGE(ArmyAnimationSequence, i8) m_animationSequence;
    i8 m_animationFrame;
    H1_ENUM_STORAGE(ArmyFacing, i8) m_facing;
    // Walk sets +-16 on diagonal moves; DrawToBuffer shifts y by frame * step.
    i16 m_walkYStep;
    i16 m_initialQuantity;
    i16 m_quantity;
    i16 m_hitPointsLost;
    // DamageEnemy: 3 rolls maximum damage, 1 minimum, 2 halves it.
    H1_ENUM_STORAGE(ArmyDamageMode, i8) m_damageMode;
    // Init copies the creature speed here; m_stats.speed is the current one.
    i8 m_baseSpeed;
    // CheckLuck: 1 good luck, -1 bad luck this attack.
    H1_ENUM_STORAGE(ArmyLuck, i8) m_luck;
    // Creature record bytes +0xc..+0x1e (hit points through attributes);
    // Init adds the hero's two primary skills to attack and defense.
    // Attribute bit 0 is a two-hex creature, bit 1 a flyer.
    tag_monsterStats m_stats;
    i16 m_unknown29;
    // PowEffect frames left on the stack: 4 hit, 5 killed, -1 none.
    i16 m_powFrames;
    i8 m_side;
    i8 m_index;
    // SpellEffect's running effect index (-1 none); DrawFrame grows the
    // redraw box upward for effects 22-25.
    i32 m_effectAnimation;
    // DrawToBuffer adds the shadow frames while set; Walk clears it to
    // redraw the field under the moving stack.
    i8 m_drawShadow;
    // combatManager::ResetHitByCreature clears it; DoHydraAttack hits
    // each stack once.
    i8 m_hitByCreature;
    class icon* m_standIcon;
    class icon* m_walkIcon;
    class icon* m_attackIcon;
    // move, attack, wince and shoot sounds.
    class sample* m_samples[ARMY_SAMPLE_COUNT];
    // Active spell; a stack carries one timed effect.
    i8 m_spellEffect;
    // What breaks m_spellEffect early: 0 the stack moving, 1 its own attack,
    // 2 taking damage, 3 only the round count; -1 with no spell.
    H1_ENUM_STORAGE(ArmySpellCancelType, i8) m_spellEndCondition;
    // ResetRound counts this down and expires the effect at zero.
    i8 m_spellRounds;
    // --- constructors ---
    army(void);
    // DoSurrender inlines this test.
    i32 IsAlive(void) {
        return m_creatureType >= 0 && m_quantity > 0;
    }
    // --- methods ---
    void InitClean(void);
    void Init(i8 creatureType, i16 quantity, i8 side, i8 index);
    void LoadResources(void);
    void FreeResources(void);
    void DrawToBuffer(i16 x, i16 y);
    // Back to the standing frame, optionally redrawing the combat screen.
    void Stand(i8 redraw);
    void Wince(void);
    // Direction, then the stand-after and continued-walk flags.
    void Walk(i16 direction, i8 standAfter, i8 continued);
    void SpecialAttack(void);
    void DirDoAttack(i16 direction);
    void DoHydraAttack(void);
    // Nonzero for a retaliation strike.
    void DoAttack(i32 retaliation);
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
        i32 rangedAttack,
        i32 defenseModifier
    );
    i32 Damage(i32 damage);
    void PowEffect(i8 effect);
    u32 Strength(void);
    void SpellEffect(i16 effect, i32 frameDelay);
    void GoBerserk(void);
    void MoveAttack(i32 destination, i32 moveOnly);
    // Undoes m_spellEffect when it expires.
    void CancelSpell(void);
    i16 CanFit(i16* hex);
    i16 ValidFlight(i16 destination, i8 pathMode);
    i16 FlyTo(void);
    i16 FlyTo(i16 destination);
    i16 FindPath(i16 sourceHex, i16 targetHex, i8 speed, i8 ignoreSpeed, i8 pathMode);
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

// Spells pass over the stack: green dragons and anti-magic ignore them
// (SPELLS' damage spells, SPELLAI's valuations); type first, then effect.
#define ARMY_IGNORES_SPELLS(a)                                                                     \
    ((a)->m_creatureType == CREATURE_DRAGON || (a)->m_spellEffect == SPELL_ANTI_MAGIC)

i16 GetAdjacentCellIndexNoArmy(i16 hex, i16 direction);
// The combat spell-effect icon cache (KB.h gCurLoadedSpellIcon): army draws
// and PowEffect share one icon, reloaded when the effect file changes.
// Stale alias of gCurLoadedSpellFileId (0x4c6d64, declared with combatManager);
// unreferenced.
extern i16 gCurLoadedSpellEffect;
// DamageEnemy flags a genie halving the target stack.
extern i8 gGenieHalf;
// DrawToBuffer's outline colours (palette indices FillToBuffer paints the
// sprite with): the stack m_limitCreature highlights, a beneficial spell
// (haste, bless, protection, anti-magic) and any other spell. SpecialAttack
// saves a MISSILE_PATCH_WIDTH x MISSILE_PATCH_HEIGHT screen patch centred on
// the missile (half sizes either side) and restores it each step.
H1_ENUM_CONST_BEGIN(ArmyDrawingConstant)
    ARMY_LIMIT_OUTLINE_COLOR = 0xe4,
    ARMY_GOOD_SPELL_OUTLINE_COLOR = 0xf7,
    ARMY_BAD_SPELL_OUTLINE_COLOR = 0xe0,
    ARMY_MISSILE_PATCH_WIDTH = 70,
    ARMY_MISSILE_PATCH_HEIGHT = 60,
    ARMY_MISSILE_HALF_WIDTH = 35,
    ARMY_MISSILE_HALF_HEIGHT = 30
H1_ENUM_CONST_END(ArmyDrawingConstant)

H1_ENUM_CONST_BEGIN(ArmyMessageConstant)
    ARMY_TARGET_NAME_SIZE = 100
H1_ENUM_CONST_END(ArmyMessageConstant)

// Creature specials fire on one outcome of SRandom(1, ROLL_MAX): the
// cyclops' paralysis and the unicorn's blindness on HIT, the genie's
// halving on GENIE_HIT. DrawToBuffer clips edge stacks from EDGE_CLIP_FRAME.
H1_ENUM_CONST_BEGIN(ArmySpecialConstant)
    ARMY_SPECIAL_ROLL_MAX = 5,
    ARMY_SPECIAL_ROLL_HIT = 3,
    ARMY_GENIE_ROLL_HIT = 2,
    ARMY_EDGE_CLIP_FRAME = 2,
    ARMY_EDGE_CLIP_LATER_FRAME = 3
H1_ENUM_CONST_END(ArmySpecialConstant)

#endif // HOMM1_SOURCE_ARMY_H

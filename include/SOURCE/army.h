#ifndef HOMM1_SOURCE_ARMY_H
#define HOMM1_SOURCE_ARMY_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 57 methods, 0 own-virtual, 0 static data.

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

// army::m_spellEndCondition: what ends m_spellEffect early (Buka
// ArmySpellCancelType numbering); Init and CancelSpell store NONE.
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

// LoadResources' sample playback settings and DrawToBuffer's quantity text
// buffer (Buka ArmyCombatConstant ARMY_SAMPLE_VOLUME/CHANNEL and
// ARMY_QUANTITY_TEXT_SIZE).
H1_ENUM_CONST_BEGIN(ArmyCombatConstant)
    ARMY_SAMPLE_VOLUME = 0x40,
    ARMY_SAMPLE_CHANNEL = 3,
    ARMY_QUANTITY_TEXT_SIZE = 12,
    // WalkTo/AttackTo when no path reaches the target (Buka ARMY_PATH_BLOCKED).
    ARMY_PATH_BLOCKED = 3,
    // SpecialAttack: a shot across an intact or damaged castle wall adds
    // this to the target's defense (DamageEnemy's defense modifier).
    ARMY_CASTLE_WALL_DEFENSE_BONUS = 4,
    // Protection's defense bonus; CancelSpell takes it back (Buka
    // ArmySpellStatConstant STONESKIN_DEFENSE_BONUS, the same +3).
    ARMY_PROTECTION_DEFENSE_BONUS = 3
H1_ENUM_CONST_END(ArmyCombatConstant)

// army::m_luck: CheckLuck rolls good or bad luck for this attack;
// DamageEnemy doubles or halves the damage and resets it.
H1_ENUM_BEGIN(ArmyLuck)
    ARMY_LUCK_BAD = -1,
    ARMY_LUCK_NONE = 0,
    ARMY_LUCK_GOOD = 1
H1_ENUM_END(ArmyLuck)

// HoMM1 combat stack, 0x54 bytes (retail constructor 0x00466490);
// army::Init copies 0x13 bytes of gMonsterDatabase from +0xc into +0x16.
// Forget an army's attack target (Buka 2.1 army.h). VC4 rejects an
// assignment through (*this).member, so the army is passed by pointer.
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
    // DrawToBuffer: pose 0 stand, 1 walk, 2 attack, 3 spell effect
    // (Buka m_animationSequence) and the frame within it.
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
    // Active spell; HoMM1 lets a stack carry one timed effect.
    i8 m_spellEffect;
    // What breaks m_spellEffect early: 0 the stack moving, 1 its own attack,
    // 2 taking damage, 3 only the round count; -1 with no spell.
    H1_ENUM_STORAGE(ArmySpellCancelType, i8) m_spellEndCondition;
    // ResetRound counts this down and expires the effect at zero.
    i8 m_spellRounds;
    // --- constructors ---
    army(void);
    // DoSurrender inlines this test (retail jmp $+0 and dead flag test).
    i32 IsAlive(void) {
        return m_creatureType >= 0 && m_quantity > 0;
    }
    // --- methods ---
    void InitClean(void);
    // HoMM1 retail: byte type, word count, byte side and index (ret 0x10).
    void Init(i8 type, i16 quantity, i8 side, i8 index);
    void LoadResources(void);
    void FreeResources(void);
    // HoMM1 retail: word x/y (ret 8).
    void DrawToBuffer(i16 x, i16 y);
    // HoMM1 retail 0x00467281: back to the standing frame, optionally
    // redrawing the combat screen (ret 4).
    void Stand(i8 redraw);
    void Wince(void);
    // HoMM1 retail 0x00467345: word direction, byte stand-after and
    // continued-walk flags (ret 0xc).
    void Walk(i16 direction, i8 standAfter, i8 continued);
    void SpecialAttack(void);
    // HoMM1 retail 0x00468fc6: word direction (ret 4).
    void DirDoAttack(i16 direction);
    // HoMM1 retail 0x00468861 takes no argument.
    void DoHydraAttack(void);
    // HoMM1 retail 0x00468ff3: nonzero for a retaliation strike (ret 4).
    void DoAttack(i32 retaliation);
    void ResetPath(void);
    i16 WalkTo(void);
    // HoMM1 retail 0x0046a0f0 / 0x0046a213: word hex, word result.
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
    // HoMM1 retail 0x0046a8d3 takes only the damage (ret 4).
    i32 Damage(i32 damage);
    // HoMM1 retail 0x0046aa49: byte effect index (ret 4).
    void PowEffect(i8 effect);
    u32 Strength(void);
    i32 LeaveNoBody(void);
    void ProcessDeath(i32 immediate);
    // HoMM1 retail 0x0046b326: word effect, frame delay (ret 8).
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
    // HoMM1 retail 0x0046b457: undoes m_spellEffect when it expires.
    void CancelSpell(void);
    void Cure(i32 amount);
    i32 MidX(void);
    i32 MidY(void);
    i32 TopY(void);
    i32 RightX(void);
    i32 LeftX(void);
    i32 OtherArmyAdjacent(i32 side, i32 index);
    i32 GetPowBaseY(void);
    // HoMM1 retail 0x0044a5e0: hex through a word pointer, word result (ret 4).
    i16 CanFit(i16* hex);
    i16 ValidFlight(i16 destination, i8 useDestination);
    // HoMM1 retail 0x0044acaf/0x0044acd6: word destination, word result.
    i16 FlyTo(void);
    i16 FlyTo(i16 destination);
    // HoMM1 retail 0x0046f280: word hexes, byte speed/flags (ret 0x14).
    i16 FindPath(i16 sourceHex, i16 targetHex, i8, i8 ignoreSpeed, i8 pathMode);
    // HoMM1 retail 0x0046f3d2: word hex, byte path mode, word result (ret 8).
    i16 ValidPath(i16 targetHex, i8 pathMode);
    i16 GetMoveMask(i16 sourceHex);
    // HoMM1 retail 0x0046f4eb: word hex, byte mode and target (ret 0xc).
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
// The combat spell-effect icon cache (KB.h gCurLoadedSpellIcon): army draws
// and PowEffect share one icon, reloaded when the effect file changes.
// Stale alias of gCurLoadedSpellFileId (0x4c6d64, declared with combatManager):
// unreferenced, kept so later symbol handles stay put.
extern i16 gCurLoadedSpellEffect;
// DamageEnemy flags a genie halving the target stack.
extern i8 gGenieHalf;
// Set while SpecialAttack fires the second shot of a double shooter.
#endif // HOMM1_SOURCE_ARMY_H

#ifndef HOMM1_SOURCE_ARMY_H
#define HOMM1_SOURCE_ARMY_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 57 methods, 0 own-virtual, 0 static data.

#include <H1/Macros.h>
#include <H1/Types.h>
#include <SOURCE/creatureTypes.h>
#include <SOURCE/spellTypes.h>

// clang-format off
// tag_monsterStats::attributes bits, which army::Init copies into each
// stack's m_stats. Buka 2.1 KB_TYPES.h MonsterFlags numbering; HoMM1 uses
// each bit as Buka does: LoadResources loads .atk/shoot sounds only for
// shooters, FLY tests flyers, the breath bit selects the two-hex DoAttack
// path (dragons, phoenixes), army::Damage/PowEffect set DEAD, CMBTMGR's
// good-morale bonus sets HIGH_MORALE, DoAttack sets RETALIATED (griffins
// excepted) and the turn code sets TURN_SPENT.
H1_ENUM_FLAGS_BEGIN(MonsterFlags, int)
    MONSTER_FLAGS_NONE = 0x0,
    MONSTER_FLAGS_WIDE = 0x1,
    MONSTER_FLAGS_FLYING = 0x2,
    MONSTER_FLAGS_SHOOTER = 0x4,
    MONSTER_FLAGS_BREATH_ATTACK = 0x8,
    MONSTER_FLAGS_DEAD = 0x10,
    MONSTER_FLAGS_HIGH_MORALE = 0x20,
    MONSTER_FLAGS_RETALIATED = 0x40,
    MONSTER_FLAGS_TURN_SPENT = 0x80
H1_ENUM_FLAGS_END(MonsterFlags)

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

// army::m_facing, passed as the icon mirror flag. Init sets side ^ 1, so
// attackers face right; a wide stack facing right has its tail at hex - 1
// (Buka combatTypes.h ArmyFacing).
H1_ENUM_BEGIN(ArmyFacing)
    ARMY_FACING_LEFT = 0,
    ARMY_FACING_RIGHT = 1
H1_ENUM_END(ArmyFacing)

// army::m_spellEndCondition: what ends m_spellEffect early (Buka
// ArmySpellCancelType numbering); Init and CancelSpell store NONE.
H1_ENUM_BEGIN(ArmySpellCancelType)
    ARMY_CANCEL_SPELLS_NONE = -1,
    ARMY_CANCEL_SPELLS_AFTER_MOVE = 0,
    ARMY_CANCEL_SPELLS_AFTER_ATTACK = 1,
    ARMY_CANCEL_SPELLS_AFTER_DAMAGE = 2,
    ARMY_CANCEL_SPELLS_ROUNDS_ONLY = 3
H1_ENUM_END(ArmySpellCancelType)

// army::SpellEffect's effect index into gCombatFxNames (and
// m_effectAnimation): 0..21 follow the spell that casts them; the luck and
// morale effects come after (rainbluk, cloudluk, moraleg, moraleb.icn).
// DrawFrame grows the redraw box upward for these four.
H1_ENUM_CONST_BEGIN(ArmyEffectAnimationConstant)
    ARMY_EFFECT_NONE = -1,
    ARMY_EFFECT_GOOD_LUCK = 22,
    ARMY_EFFECT_BAD_LUCK = 23,
    ARMY_EFFECT_GOOD_MORALE = 24,
    ARMY_EFFECT_BAD_MORALE = 25,
    ARMY_EFFECT_COUNT = 26
H1_ENUM_CONST_END(ArmyEffectAnimationConstant)

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
    ARMY_QUANTITY_TEXT_SIZE = 12
H1_ENUM_CONST_END(ArmyCombatConstant)
// clang-format on

// HoMM1 combat stack, 0x54 bytes (retail constructor 0x00466490);
// army::Init copies 0x13 bytes of gMonsterDatabase from +0xc into +0x16.
#pragma pack(push, 1)
class army {
public:
    // Attack target (GetCommand clears both to -1).
    signed char m_targetSide;
    signed char m_targetIndex;
    // ValidRange records the chosen attack direction.
    short m_attackDirection;
    signed char m_unknown04;
    // ValidPath records the reachable target hex here.
    signed char m_moveTargetHex;
    H1_ENUM_STORAGE(CreatureType, signed char) m_creatureType;
    signed char m_hex;
    // DrawToBuffer: pose 0 stand, 1 walk, 2 attack, 3 spell effect
    // (Buka m_animationSequence) and the frame within it.
    H1_ENUM_STORAGE(ArmyAnimationSequence, signed char) m_animationSequence;
    signed char m_animationFrame;
    H1_ENUM_STORAGE(ArmyFacing, signed char) m_facing;
    // Walk sets +-16 on diagonal moves; DrawToBuffer shifts y by frame * step.
    short m_walkYStep;
    short m_initialQuantity;
    short m_quantity;
    short m_hitPointsLost;
    // DamageEnemy: 3 rolls maximum damage, 1 minimum, 2 halves it.
    H1_ENUM_STORAGE(ArmyDamageMode, signed char) m_damageMode;
    // Init copies the creature speed here; m_stats.speed is the current one.
    signed char m_baseSpeed;
    // CheckLuck: 1 good luck, -1 bad luck this attack.
    signed char m_luck;
    // Creature record bytes +0xc..+0x1e (hit points through attributes);
    // Init adds the hero's two primary skills to attack and defense.
    // Attribute bit 0 is a two-hex creature, bit 1 a flyer.
    tag_monsterStats m_stats;
    short m_unknown29;
    // PowEffect frames left on the stack: 4 hit, 5 killed, -1 none.
    short m_powFrames;
    signed char m_side;
    signed char m_index;
    // SpellEffect's running effect index (-1 none); DrawFrame grows the
    // redraw box upward for effects 22-25.
    int m_effectAnimation;
    // DrawToBuffer adds the shadow frames while set; Walk clears it to
    // redraw the field under the moving stack.
    signed char m_drawShadow;
    // combatManager::ResetHitByCreature clears it; DoHydraAttack hits
    // each stack once.
    char m_hitByCreature;
    class icon* m_standIcon;
    class icon* m_walkIcon;
    class icon* m_attackIcon;
    // move, attack, wince and shoot sounds.
    class sample* m_samples[ARMY_SAMPLE_COUNT];
    // Active spell; HoMM1 lets a stack carry one timed effect.
    signed char m_spellEffect;
    // What breaks m_spellEffect early: 0 the stack moving, 1 its own attack,
    // 2 taking damage, 3 only the round count; -1 with no spell.
    H1_ENUM_STORAGE(ArmySpellCancelType, signed char) m_spellEndCondition;
    // ResetRound counts this down and expires the effect at zero.
    signed char m_spellRounds;
    // --- constructors ---
    army(void);
    // DoSurrender inlines this test (retail jmp $+0 and dead flag test).
    int IsAlive(void) {
        return m_creatureType >= 0 && m_quantity > 0;
    }
    // --- methods ---
    void WaitSample(int);
    void InitClean(void);
    // HoMM1 retail: byte type, word count, byte side and index (ret 0x10).
    void Init(signed char, short, signed char, signed char);
    void LoadResources(void);
    void FreeResources(void);
    // HoMM1 retail: word x/y (ret 8).
    void DrawToBuffer(short, short);
    // HoMM1 retail 0x00467281: back to the standing frame, optionally
    // redrawing the combat screen (ret 4).
    void Stand(signed char);
    void Wince(void);
    // HoMM1 retail 0x00467345: word direction, byte stand-after and
    // continued-walk flags (ret 0xc).
    void Walk(short, signed char, signed char);
    void SpecialAttack(void);
    // HoMM1 retail 0x00468fc6: word direction (ret 4).
    void DirDoAttack(short);
    // HoMM1 retail 0x00468861 takes no argument.
    void DoHydraAttack(void);
    // HoMM1 retail 0x00468ff3: nonzero for a retaliation strike (ret 4).
    void DoAttack(int);
    void ResetPath(void);
    short WalkTo(void);
    // HoMM1 retail 0x0046a0f0 / 0x0046a213: word hex, word result.
    short WalkTo(short);
    short AttackTo(void);
    short AttackTo(short);
    void CheckLuck(void);
    void DamageEnemy(class army*, int*, int*, int, int);
    // HoMM1 retail 0x0046a8d3 takes only the damage (ret 4).
    int Damage(long int);
    // HoMM1 retail 0x0046aa49: byte effect index (ret 4).
    void PowEffect(signed char);
    unsigned long int Strength(void);
    int LeaveNoBody(void);
    void ProcessDeath(int);
    // HoMM1 retail 0x0046b326: word effect, frame delay (ret 8).
    void SpellEffect(short, int);
    void CancelSpellType(int);
    void CancelIndividualSpell(int);
    int SetSpellInfluence(int, int);
    void DecrementSpellRounds(void);
    void GoBerserk(void);
    void MoveAttack(int, int);
    float SpellCastWorkChance(int);
    int SpellCastWorks(int);
    void DispelGood(void);
    // HoMM1 retail 0x0046b457: undoes m_spellEffect when it expires.
    void CancelSpell(void);
    void Cure(int);
    int MidX(void);
    int MidY(void);
    int TopY(void);
    int RightX(void);
    int LeftX(void);
    int OtherArmyAdjacent(int, int);
    int GetPowBaseY(void);
    // HoMM1 retail 0x0044a5e0: hex through a word pointer, word result (ret 4).
    short CanFit(short*);
    short ValidFlight(short, signed char);
    // HoMM1 retail 0x0044acaf/0x0044acd6: word destination, word result.
    short FlyTo(void);
    short FlyTo(short);
    // HoMM1 retail 0x004180f0: word hexes, byte speed/flags (ret 0x14).
    short FindPath(short, short, signed char, signed char, signed char);
    // HoMM1 retail 0x00418242: word hex, byte path mode, word result (ret 8).
    short ValidPath(short, signed char);
    short GetMoveMask(short);
    // HoMM1 retail 0x0041835b: word hex, byte mode and target (ret 0xc).
    short GetAttackMask(short, signed char, signed char);
    short ValidMove(short);
    short ValidMove(short, short);
    short ValidAttack(short, short, short, short, short*);
    short GetAdjacentCellIndex(short, short);
    short ValidRange(short);
    short GetBestDirection(short, short, short);
};
#pragma pack(pop)

short GetAdjacentCellIndexNoArmy(short, short);
// The combat spell-effect icon cache (KB.h gCurLoadedSpellIcon): army draws
// and PowEffect share one icon, reloaded when the effect file changes.
// Stale alias of gCurLoadedSpellFileId (0x4c6d64, declared with combatManager):
// unreferenced, kept so later symbol handles stay put.
extern short gCurLoadedSpellEffect;
extern short giSpellEffectFrame;
// Pow (impact) effect icons by effect (0x00491098).
extern char* gPowEffectNames[];
// Damage multipliers for attack minus defense, -20..20 (0x00492470).
extern float gfBattleStat[];
// DamageEnemy flags a genie halving the target stack.
extern signed char gbGenieHalf;
// Set while SpecialAttack fires the second shot of a double shooter.
extern int gbSecondShot;
#endif // HOMM1_SOURCE_ARMY_H

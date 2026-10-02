#ifndef HOMM1_SOURCE_ARMY_H
#define HOMM1_SOURCE_ARMY_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 57 methods, 0 own-virtual, 0 static data.

#include <H1/Macros.h>
#include <H1/Types.h>

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
    signed char m_creatureType;
    signed char m_hex;
    // DrawToBuffer: pose 0 stand, 1 walk, 2 attack, 3 spell effect
    // (Buka m_animationSequence) and the frame within it.
    signed char m_animationSequence;
    signed char m_animationFrame;
    signed char m_facing;
    // Walk sets +-16 on diagonal moves; DrawToBuffer shifts y by frame * step.
    short m_walkYStep;
    short m_initialQuantity;
    short m_quantity;
    short m_hitPointsLost;
    // DamageEnemy: 3 rolls maximum damage, 1 minimum, 2 halves it.
    signed char m_damageMode;
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
    class sample* m_samples[4];
    // Active spell; HoMM1 lets a stack carry one timed effect.
    signed char m_spellEffect;
    // What breaks m_spellEffect early: 0 the stack moving, 1 its own attack,
    // 2 taking damage, 3 only the round count; -1 with no spell.
    signed char m_spellEndCondition;
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
extern short gCurLoadedSpellFileId;
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

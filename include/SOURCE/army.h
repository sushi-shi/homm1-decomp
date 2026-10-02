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
    signed char m_unknown08;
    signed char m_unknown09;
    signed char m_facing;
    short m_unknown0b;
    short m_initialQuantity;
    short m_quantity;
    short m_hitPointsLost;
    signed char m_unknown13;
    // Init copies the creature speed here; m_stats.speed is the current one.
    signed char m_baseSpeed;
    signed char m_unknown15;
    // Creature record bytes +0xc..+0x1e (hit points through attributes);
    // Init adds the hero's two primary skills to attack and defense.
    // Attribute bit 0 is a two-hex creature, bit 1 a flyer.
    tag_monsterStats m_stats;
    short m_unknown29;
    short m_unknown2b;
    signed char m_side;
    signed char m_index;
    int m_unknown2f;
    signed char m_unknown33;
    char m_unknown34;
    class icon* m_standIcon;
    class icon* m_walkIcon;
    class icon* m_attackIcon;
    // move, attack, wince and shoot sounds.
    class sample* m_samples[4];
    // Active spell; HoMM1 lets a stack carry one timed effect.
    signed char m_spellEffect;
    signed char m_unknown52;
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
    void Wince(void);
    void Walk(int, int, int);
    void SpecialAttack(void);
    void DirDoAttack(int);
    void DoHydraAttack(int);
    void DoAttack(int);
    void ResetPath(void);
    short WalkTo(void);
    short WalkTo(short);
    short AttackTo(void);
    short AttackTo(short);
    void CheckLuck(void);
    void DamageEnemy(class army *, int *, int *, int, int);
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
    // HoMM1 retail 0x00467281: back to the standing frame, regrid and
    // optionally redraw (ret 4).
    void ResetAnimation(signed char);
    void Cure(int);
    int MidX(void);
    int MidY(void);
    int TopY(void);
    int RightX(void);
    int LeftX(void);
    int OtherArmyAdjacent(int, int);
    int GetPowBaseY(void);
    // HoMM1 retail 0x0044a5e0: hex through a word pointer, word result (ret 4).
    short CanFit(short *);
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
    short ValidAttack(short, short, short, short, short *);
    short GetAdjacentCellIndex(short, short);
    short ValidRange(short);
    short GetBestDirection(short, short, short);
};
#pragma pack(pop)

short GetAdjacentCellIndexNoArmy(short, short);
#endif // HOMM1_SOURCE_ARMY_H

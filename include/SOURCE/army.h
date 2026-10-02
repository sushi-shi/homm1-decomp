#ifndef HOMM1_SOURCE_ARMY_H
#define HOMM1_SOURCE_ARMY_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 57 methods, 0 own-virtual, 0 static data.

#include <H1/Macros.h>
#include <H1/Types.h>

// HoMM1 combat stacks are 0x54 bytes: combatManager strides sides by six
// armies (0x1f8) from +0x2ca, and Init copies 0x13 bytes of the creature
// record from +0xc.
#pragma pack(push, 1)
class army {
public:
    signed char m_unknown00;
    signed char m_unknown01;
    short m_unknown02;
    signed char m_unknown04;
    // WalkTo()/AttackTo() forward this hex to their one-argument forms.
    signed char m_targetHex;
    signed char m_monsterType;
    signed char m_hex;
    signed char m_unknown08;
    signed char m_unknown09;
    // hexcell::DrawOccupant redraws the stack when its cached frame differs.
    signed char m_facing;
    short m_unknown0b;
    short m_initialQuantity;
    short m_quantity;
    short m_hitPointsLost;
    signed char m_unknown13;
    signed char m_speed;
    signed char m_unknown15;
    // Creature record bytes +0xc..+0x1e (hit points through attributes).
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
    signed char m_unknown51;
    signed char m_unknown52;
    char m_unknown53;
    // --- constructors ---
    army(void);
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
    int Damage(long int, int);
    void PowEffect(int, int, int, int);
    unsigned long int Strength(void);
    int LeaveNoBody(void);
    void ProcessDeath(int);
    void SpellEffect(int, int, int);
    void CancelSpellType(int);
    void CancelIndividualSpell(int);
    int SetSpellInfluence(int, int);
    void DecrementSpellRounds(void);
    void GoBerserk(void);
    void MoveAttack(int, int);
    float SpellCastWorkChance(int);
    int SpellCastWorks(int);
    void DispelGood(void);
    void Cure(int);
    int MidX(void);
    int MidY(void);
    int TopY(void);
    int RightX(void);
    int LeftX(void);
    int OtherArmyAdjacent(int, int);
    int GetPowBaseY(void);
    int CanFit(int, int, int *);
    int ValidFlight(int, int);
    int FlyTo(void);
    int FlyTo(int);
    int FindPath(int, int, int, int, int);
    int ValidPath(int, int);
    int GetMoveMask(int);
    int GetAttackMask(int, int, int);
    int ValidMove(int);
    int ValidMove(int, int);
    int ValidAttack(int, int, int, int, int *);
    int GetAdjacentCellIndex(int, int);
    int ValidRange(int);
    int GetBestDirection(int, int, int);
};
#pragma pack(pop)
#endif // HOMM1_SOURCE_ARMY_H

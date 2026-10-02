#ifndef HOMM1_SOURCE_ARMY_H
#define HOMM1_SOURCE_ARMY_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 57 methods, 0 own-virtual, 0 static data.

#include <H1/Macros.h>

// HoMM1 combat stack, 0x54 bytes (retail constructor 0x00466490);
// army::Init copies 0x13 bytes of gMonsterDatabase from +0xc into +0x16.
#pragma pack(push, 1)
class army {
public:
    signed char m_unknown00;
    signed char m_unknown01;
    short m_unknown02;
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
    signed char m_baseSpeed;
    char m_unknown15;
    signed char m_race;
    signed char m_speed;
    signed char m_unknown18;
    signed char m_unknown19;
    signed char m_unknown1a;
    signed char m_unknown1b;
    signed char m_unknown1c;
    char m_spriteName[8];
    // Monster attribute flags; bit 0 is a two-hex creature, bit 1 a flyer.
    int m_attributes;
    short m_unknown29;
    short m_unknown2b;
    signed char m_side;
    signed char m_index;
    int m_unknown2f;
    signed char m_unknown33;
    char m_unknown34;
    int m_unknown35;
    int m_unknown39;
    int m_unknown3d;
    int m_unknown41[4];
    // Active spell; HoMM1 lets a stack carry one timed effect.
    signed char m_spellEffect;
    signed char m_unknown52;
    // ResetRound counts this down and expires the effect at zero.
    signed char m_spellRounds;
    // --- constructors ---
    army(void);
    // --- methods ---
    void WaitSample(int);
    void InitClean(void);
    void Init(int, int, int, int, int, int);
    void LoadResources(void);
    void FreeResources(void);
    void DrawToBuffer(int, int, int);
    void Wince(void);
    void Walk(int, int, int);
    void SpecialAttack(void);
    void DirDoAttack(int);
    void DoHydraAttack(int);
    void DoAttack(int);
    void ResetPath(void);
    int WalkTo(void);
    int WalkTo(int);
    int AttackTo(void);
    int AttackTo(int);
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

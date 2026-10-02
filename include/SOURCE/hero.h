#ifndef HOMM1_SOURCE_HERO_H
#define HOMM1_SOURCE_HERO_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 34 methods, 0 own-virtual, 0 static data.

#include <H1/Macros.h>
#include <SOURCE/armyGroup.h>

// forward declarations:
class town;

// DemobilizeCurrHero's unfolded load/or/store proves an unsigned flag
// operand against the signed event-flag dword.
#define HERO_EVENT_EMBARKED 0x80u

// Retail strides hero records by 0xb6 bytes from game+0x12985; the tail
// keeps HoMM2's event-flag dword and AI fight-value float.
#pragma pack(push, 1)
class hero {
public:
    signed char m_id;
    signed char m_owner;
    char m_name[0x1a];
    signed char m_unknown1c;
    char m_unknown1d;
    signed char m_x;
    signed char m_y;
    signed char m_destinationX;
    signed char m_destinationY;
    unsigned char m_direction;
    unsigned char m_locationType;
    // SetHeroContext passes it zero-extended to game::RestoreCell.
    unsigned char m_occupiedTown;
    short m_mobility;
    short m_remainingMobility;
    int m_experience;
    char m_unknown2d[0x2a];
    // philAI::CombatMonsterEvent passes &m_army to QuickCombat.
    armyGroup m_army;
    char m_unknown66[0x3a];
    signed char m_artifacts[14];
    int m_eventFlags;
    float m_aiFightValue;
    // --- constructors ---
    hero(void);
    // --- methods ---
    void Read(int, signed char);
    void Write(int, signed char);
    void GetArmyStrengths(unsigned long int * const);
    // HoMM1 returns the flag in AL (townManager::Main sign-extends it).
    signed char HasArtifact(int);
    int CalcMobility(void);
    int HasSpell(int);
    int GetNthSpell(int, int);
    int GetNumSpells(int);
    void UseSpell(signed char);
    void AddSpell(int, int);
    void HeroScreenUpdate(void);
    signed char HeroView(signed char);
    void UpdateArmies(void);
    void ViewStat(int, int);
    void ViewArtifact(int, int, int);
    signed char Dismiss(void);
    void Deallocate(void);
    int GetExperience(int);
    int GetLevel(int);
    void ApplyBattleWinTemps(void);
    void ApplyBattleLossTemps(void);
    void CheckLevel(void);
    int NumArtifacts(void);
    void SetSS(int, int);
    int TakeSS(int, int);
    int GiveSS(int, int);
    int CreatureTypeCount(int);
    void UpgradeCreatures(int, int);
    int GetNthSS(int);
    class town * GetOccupiedTown(void);
    signed char Stats(int);
    signed char GetSSLevel(int);
    void DoSSLevelDialog(int, int);
    void CheckAnduranPieces(int);
};
#pragma pack(pop)
// Hero type names, indexed by hero::m_unknown1c (retail 0x00493240).
extern char *cHeroTypeName[];
#endif // HOMM1_SOURCE_HERO_H

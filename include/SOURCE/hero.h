#ifndef HOMM1_SOURCE_HERO_H
#define HOMM1_SOURCE_HERO_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 34 methods, 0 own-virtual, 0 static data.

#include <Domains.h>
#include <H1/Macros.h>
#include <SOURCE/armyGroup.h>

// forward declarations:
class town;

// DemobilizeCurrHero's unfolded load/or/store proves an unsigned flag
// operand against the signed event-flag dword.
#define HERO_EVENT_EMBARKED 0x80u
// ApplyBattleWinTemps' unfolded load/sub/store likewise proves unsigned
// battle-temporary visit flags.
#define HERO_EVENT_BUOY 0x2u
#define HERO_EVENT_FOUNTAIN 0x4u
#define HERO_EVENT_OASIS 0x8u
#define HERO_EVENT_FAERIE_RING 0x10u
#define HERO_EVENT_GRAVEYARD 0x20u
#define HERO_EVENT_SHIPWRECK 0x40u
#define HERO_EVENT_TEMPLE 0x100u

// clang-format off
H1_ENUM_BEGIN(HeroConstant)
    HERO_PRIMARY_STAT_COUNT = 4,
    HERO_STARTING_STAT_COUNT = 5,
    HERO_COMBAT_SPELL_SLOT_COUNT = 19,
    HERO_SPELL_SLOT_COUNT = 29,
    HERO_ARTIFACT_SLOT_COUNT = 14,
    HERO_EXPERIENCE_LEVEL_TABLE_COUNT = 12
H1_ENUM_END(HeroConstant)


// Artifact ids follow HoMM1's artifact table order.
H1_ENUM_BEGIN(ArtifactType)
    ARTIFACT_NOMAD_BOOTS = 29,
    ARTIFACT_TRAVELER_BOOTS = 30,
    ARTIFACT_TRUE_COMPASS = 35,
    ARTIFACT_SAILORS_ASTROLABE = 36,
    ARTIFACT_MAGIC_BOOK = 37
H1_ENUM_END(ArtifactType)
// clang-format on

// Retail strides hero records by 0xb6 bytes from game+0x12985; the tail
// keeps HoMM2's event-flag dword and AI fight-value float.
#pragma pack(push, 1)
class hero {
public:
    // CheckLevel and Deallocate sign-extend the hero id.
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
    signed char m_occupiedTown;
    short m_mobility;
    short m_remainingMobility;
    int m_experience;
    char m_unknown2d;
    short m_level;
    // GiveTakeArtifactStat raises a fifth stat byte for artifact 17.
    signed char m_primaryStats[HERO_STARTING_STAT_COUNT];
    signed char m_morale;
    signed char m_luck;
    char m_unknown37[6];
    short m_randomSeed;
    char m_unknown3f[0x18];
    armyGroup m_army;
    // Combat spells fill the first 19 slots, adventure spells the last 10;
    // each memorized spell keeps its remaining casts in the parallel array.
    signed char m_spells[HERO_SPELL_SLOT_COUNT];
    signed char m_spellCharges[HERO_SPELL_SLOT_COUNT];
    signed char m_artifacts[HERO_ARTIFACT_SLOT_COUNT];
    int m_eventFlags;
    float m_aiFightValue;
    // --- constructors ---
    hero(void);
    // --- methods ---
    void Read(int, signed char);
    void Write(int, signed char);
    void GetArmyStrengths(unsigned long int * const);
    signed char HasArtifact(signed char);
    short CalcMobility(void);
    signed char HasSpell(signed char);
    int GetNthSpell(int, int);
    short GetNumSpells(signed char);
    void UseSpell(signed char);
    int AddSpell(signed char, signed char, int);
    void HeroScreenUpdate(void);
    void UpdateArmies(void);
    void RedrawHeroScreen(void);
    void HeroView(signed char);
    void ViewStat(signed char, signed char);
    void ViewArtifact(signed char, signed char);
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

// Per-class sea mobility multiplier and level thresholds (retail 0x492038,
// 0x492598).
extern float gfClassNavigationMod[];
extern class heroWindow* heroWin;
extern class heroWindow* gheroWin;

void HeroMessageUpdate(char*);
extern char* gStatNames[];
extern char* gStatDesc[];
extern char* gArtifactDesc[];
extern int giHeroScreenSrcIndex;
extern short gMinExpForLevel[][HERO_EXPERIENCE_LEVEL_TABLE_COUNT];
#endif // HOMM1_SOURCE_HERO_H

#ifndef HOMM1_SOURCE_HERO_H
#define HOMM1_SOURCE_HERO_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 34 methods, 0 own-virtual, 0 static data.

#include <Domains.h>
#include <H1/Macros.h>
#include <SOURCE/armyGroup.h>
#include <SOURCE/artifactTypes.h>
#include <SOURCE/spellTypes.h>

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
#define HERO_EVENT_STATUE 0x100u

H1_ENUM_CONST_BEGIN(HeroConstant)
    HERO_PRIMARY_STAT_COUNT = 4,
    HERO_STARTING_STAT_COUNT = 5,
    HERO_COMBAT_SPELL_SLOT_COUNT = 19,
    HERO_SPELL_SLOT_COUNT = 29,
    HERO_ARTIFACT_SLOT_COUNT = 14,
    HERO_EXPERIENCE_LEVEL_TABLE_COUNT = 12,
    // Hero ids run nine per class (GetNewHeroId, RecruitHero's id / 9) over
    // the four classes.
    HERO_PER_CLASS_COUNT = 9,
    HERO_CLASS_COUNT = 4,
    // Dismiss clears m_owner and the destination; playerData's hero lists
    // (m_heroIds, m_currentHero) and the boat records mark an empty entry
    // with HERO_ID_NONE (Buka hero.h HERO_OWNER_NONE / HERO_DESTINATION_NONE).
    HERO_OWNER_NONE = -1,
    HERO_DESTINATION_NONE = -1,
    HERO_ID_NONE = -1,
    // playerData::m_availableHeroIds: the tavern's two heroes for hire.
    HERO_AVAILABLE_SLOT_COUNT = 2,
    // CheckLevel seeds SRand with m_randomSeed + level * SEED_FACTOR and
    // reads gHeroSkillBonus row level - FIRST_LEVEL, clamped to ROW_LAST
    // (Buka HERO_LEVEL_RANDOM_SEED_FACTOR).
    HERO_LEVEL_RANDOM_SEED_FACTOR = 30,
    HERO_SKILL_BONUS_FIRST_LEVEL = 2,
    HERO_SKILL_BONUS_ROW_LAST = 8
H1_ENUM_CONST_END(HeroConstant)

// game::m_availableHeroes per hero id: the owning player, UNAVAILABLE, or
// RETREATED for a hero that retreated or surrendered and waits in its
// owner's tavern (Dismiss; Buka hero.h HeroConstant numbering).
H1_ENUM_CONST_BEGIN(HeroAvailability)
    HERO_AVAILABILITY_UNAVAILABLE = -1,
    HERO_AVAILABILITY_RETREATED = 0x40
H1_ENUM_CONST_END(HeroAvailability)

// hero::GetNumSpells' selector (Buka hero.h HeroSpellType): combat slots,
// adventure slots or both.
H1_ENUM_BEGIN(HeroSpellType)
    SPELL_TYPE_COMBAT = 0,
    SPELL_TYPE_ADVENTURE = 1,
    SPELL_TYPE_CATEGORY_COUNT = 2,
    SPELL_TYPE_ALL = SPELL_TYPE_CATEGORY_COUNT
H1_ENUM_END(HeroSpellType)

// m_primaryStats indices: the order of retail gPrimarySkillNames
// (0x00493210) and their help texts; army::Init adds 0 and 1 to the
// stack's attack and defense, and AddSpell receives 3 as the spell count.
// advManager::GiveTakeArtifactStat also raises the fifth byte (index 4)
// for the Ballista of Quickness, which no retail code reads (CMBTMGR tests
// the artifact itself), and uses -1 for artifacts without a stat bonus.
H1_ENUM_BEGIN(HeroPrimaryStat)
    HERO_PRIMARY_NONE = -1,
    HERO_PRIMARY_ATTACK = 0,
    HERO_PRIMARY_DEFENSE = 1,
    HERO_PRIMARY_SPELL_POWER = 2,
    HERO_PRIMARY_KNOWLEDGE = 3,
    HERO_PRIMARY_BALLISTA = 4
H1_ENUM_END(HeroPrimaryStat)

// Retail strides hero records by 0xb6 bytes from game+0x12985; the tail
// keeps HoMM2's event-flag dword and AI fight-value float.
#pragma pack(push, 1)
class hero {
public:
    // CheckLevel and Deallocate sign-extend the hero id.
    signed char m_id;
    signed char m_owner;
    char m_name[0x11];
    // UpdBottomViewHero copies this 8-character label into its name widget.
    char m_shortName[9];
    // Indexes gClassNames, gMinExpForLevel and the class crest frames;
    // combat copies it to m_heroType.
    signed char m_heroClass;
    // port%04d.icn portrait number.
    char m_portrait;
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
    char m_unknown2d;
    short m_level;
    // GiveTakeArtifactStat raises a fifth stat byte for artifact 17.
    signed char m_primaryStats[HERO_STARTING_STAT_COUNT];
    signed char m_morale;
    signed char m_luck;
    // ShowMoraleInfo reports the cowardice byte separately.
    signed char m_cowardice;
    char m_unknown38;
    // DoAIEvent tests and sets one bit per visited site index.
    int m_visitedSites;
    short m_randomSeed;
    char m_unknown3f[0x18];
    armyGroup m_army;
    // Combat spells fill the first 19 slots, adventure spells the last 10;
    // each memorized spell keeps its remaining casts in the parallel array.
    H1_ENUM_STORAGE(SpellType, signed char) m_spells[HERO_SPELL_SLOT_COUNT];
    signed char m_spellCharges[HERO_SPELL_SLOT_COUNT];
    H1_ENUM_STORAGE(ArtifactType, signed char) m_artifacts[HERO_ARTIFACT_SLOT_COUNT];
    int m_eventFlags;
    float m_aiFightValue;
    int IsEmbarked(void) {
        return m_eventFlags & HERO_EVENT_EMBARKED;
    }
    // --- constructors ---
    hero(void);
    // --- methods ---
    void Read(int file, signed char expansion);
    void Write(int file, signed char expansion);
    void GetArmyStrengths(unsigned long int* const);
    signed char HasArtifact(H1_ENUM_PARAM(ArtifactType, signed char));
    short CalcMobility(void);
    signed char HasSpell(H1_ENUM_PARAM(SpellType, signed char));
    int GetNthSpell(int type, int spellNumber);
    short GetNumSpells(H1_ENUM_PARAM(HeroSpellType, signed char));
    void UseSpell(H1_ENUM_PARAM(SpellType, signed char));
    int AddSpell(H1_ENUM_PARAM(SpellType, signed char), signed char, int);
    void HeroScreenUpdate(void);
    void UpdateArmies(void);
    void RedrawHeroScreen(void);
    signed char HeroView(signed char);
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
    void SetSS(int skill, int level);
    int TakeSS(int skill, int levels);
    int GiveSS(int skill, int levels);
    int CreatureTypeCount(int creatureType);
    void UpgradeCreatures(int oldCreatureType, int newCreatureType);
    int GetNthSS(int ordinal);
    class town* GetOccupiedTown(void);
    signed char Stats(int stat);
    signed char GetSSLevel(int skill);
    void DoSSLevelDialog(int skill, int quickView);
    void CheckAnduranPieces(int showDialog);
};
#pragma pack(pop)

extern class heroWindow* gheroWin;

void HeroMessageUpdate(char*);
void UpdateHeroScreenStatusBar(short);
// Stale alias of gbHeroWindShowing (0x494128): unreferenced, kept so later symbol handles stay put.
extern signed char gbHeroScreenActive;
short HeroHandler(struct tag_message&);
#endif // HOMM1_SOURCE_HERO_H

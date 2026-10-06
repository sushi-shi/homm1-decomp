#ifndef HOMM1_SOURCE_HERO_H
#define HOMM1_SOURCE_HERO_H

#include <SOURCE/armyGroup.h>
#include <SOURCE/artifactTypes.h>
#include <SOURCE/spellTypes.h>

class town;

#define HERO_EVENT_EMBARKED 0x80u
#define HERO_EVENT_BUOY 0x2u
#define HERO_EVENT_FOUNTAIN 0x4u
#define HERO_EVENT_OASIS 0x8u
#define HERO_EVENT_FAERIE_RING 0x10u
#define HERO_EVENT_GRAVEYARD 0x20u
#define HERO_EVENT_SHIPWRECK 0x40u
#define HERO_EVENT_STATUE 0x100u

enum HeroConstant {
    HERO_PRIMARY_STAT_COUNT = 4,
    HERO_STARTING_STAT_COUNT = 5,
    HERO_COMBAT_SPELL_SLOT_COUNT = 19,
    HERO_SPELL_SLOT_COUNT = 29,
    HERO_ARTIFACT_SLOT_COUNT = 14,
    HERO_EXPERIENCE_LEVEL_TABLE_COUNT = 12,
    HERO_PER_CLASS_COUNT = 9,
    HERO_CLASS_COUNT = 4,
    HERO_OWNER_NONE = -1,
    HERO_DESTINATION_NONE = -1,
    HERO_ID_NONE = -1,
    HERO_AVAILABLE_SLOT_COUNT = 2,
    HERO_LEVEL_RANDOM_SEED_FACTOR = 30,
    HERO_SKILL_BONUS_FIRST_LEVEL = 2,
    HERO_SKILL_BONUS_ROW_LAST = 8
};

enum HeroAvailability {
    HERO_AVAILABILITY_UNAVAILABLE = -1,
    HERO_AVAILABILITY_RETREATED = 0x40
};

enum HeroSpellType {
    SPELL_TYPE_COMBAT = 0,
    SPELL_TYPE_ADVENTURE = 1,
    SPELL_TYPE_CATEGORY_COUNT = 2,
    SPELL_TYPE_ALL = SPELL_TYPE_CATEGORY_COUNT
};

enum HeroPrimaryStat {
    HERO_PRIMARY_NONE = -1,
    HERO_PRIMARY_ATTACK = 0,
    HERO_PRIMARY_DEFENSE = 1,
    HERO_PRIMARY_SPELL_POWER = 2,
    HERO_PRIMARY_KNOWLEDGE = 3,
    HERO_PRIMARY_BALLISTA = 4
};

#pragma pack(push, 1)
class hero {
public:
    i8 m_id;
    i8 m_owner;
    char m_name[0x11];
    char m_shortName[9];
    i8 m_heroClass;
    i8 m_portrait;
    i8 m_x;
    i8 m_y;
    i8 m_destinationX;
    i8 m_destinationY;
    u8 m_direction;
    u8 m_locationType;
    u8 m_occupiedTown;
    i16 m_mobility;
    i16 m_remainingMobility;
    i32 m_experience;
    i8 m_unknown2d;
    i16 m_level;
    i8 m_primaryStats[HERO_STARTING_STAT_COUNT];
    i8 m_morale;
    i8 m_luck;
    i8 m_cowardice;
    i8 m_unknown38;
    i32 m_visitedSites;
    i16 m_randomSeed;
    char m_unknown3f[0x18];
    armyGroup m_army;
    i8 m_spells[HERO_SPELL_SLOT_COUNT];
    i8 m_spellCharges[HERO_SPELL_SLOT_COUNT];
    i8 m_artifacts[HERO_ARTIFACT_SLOT_COUNT];
    i32 m_eventFlags;
    float m_aiFightValue;
    i32 IsEmbarked(void) {
        return m_eventFlags & HERO_EVENT_EMBARKED;
    }
    hero(void);
    void Read(i32 file, i8 expansion);
    void Write(i32 file, i8 expansion);
    void GetArmyStrengths(u32* const);
    i8 HasArtifact(i8 artifact);
    i16 CalcMobility(void);
    i8 HasSpell(i8 spell);
    i32 GetNthSpell(i32 type, i32 spellNumber);
    i16 GetNumSpells(i8 type);
    void UseSpell(i8 spell);
    i32 AddSpell(i8 spell, i8 charges, i32 checkOnly);
    void HeroScreenUpdate(void);
    void UpdateArmies(void);
    void RedrawHeroScreen(void);
    i8 HeroView(i8 viewOnly);
    void ViewStat(i8 stat, i8 quickView);
    void ViewArtifact(i8 artifact, i8 quickView);
    i8 Dismiss(void);
    void Deallocate(void);
    i32 GetExperience(i32 level);
    i32 GetLevel(i32 experienceValue);
    void ApplyBattleWinTemps(void);
    void ApplyBattleLossTemps(void);
    void CheckLevel(void);
    i32 NumArtifacts(void);
    void SetSS(i32 skill, i32 level);
    i32 TakeSS(i32 skill, i32 levels);
    i32 GiveSS(i32 skill, i32 levels);
    i32 CreatureTypeCount(i32 creatureType);
    void UpgradeCreatures(i32 oldCreatureType, i32 newCreatureType);
    i32 GetNthSS(i32 ordinal);
    class town* GetOccupiedTown(void);
    i8 Stats(i32 stat);
    i8 GetSSLevel(i32 skill);
    void DoSSLevelDialog(i32 skill, i32 quickView);
    void CheckAnduranPieces(i32 showDialog);
};
#pragma pack(pop)

extern class heroWindow* gheroWin;

void HeroMessageUpdate(char* text);
void UpdateHeroScreenStatusBar(i16 widgetId);
extern i8 gbHeroScreenActive;
i16 HeroHandler(struct tag_message& message);
#endif

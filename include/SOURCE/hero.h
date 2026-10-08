#ifndef HOMM1_SOURCE_HERO_H
#define HOMM1_SOURCE_HERO_H

#include <BASE/dialog.h>
#include <BASE/message.h>
#include <SOURCE/armyGroup.h>
#include <SOURCE/artifactTypes.h>
#include <SOURCE/cursorTypes.h>
#include <SOURCE/spellTypes.h>

class town;

#define HERO_EVENT_NONE 0x0u
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
    HERO_DESTINATION_NONE = -1,
    HERO_ID_NONE = -1,
    HERO_AVAILABLE_SLOT_COUNT = 2,
    HERO_LEVEL_RANDOM_SEED_FACTOR = 30,
    HERO_SKILL_BONUS_FIRST_LEVEL = 2,
    HERO_SKILL_BONUS_ROW_LAST = 8,
    // Sorceresses and Warlocks start with a spell book.
    HERO_CLASS_FIRST_SPELLCASTER = 2,
    // m_cowardice: each lost battle that did not end in surrender lowers it
    // down to this floor; each won battle raises it back towards zero.
    HERO_COWARDICE_MIN = -3
};

enum HeroAvailability {
    HERO_AVAILABILITY_UNAVAILABLE = -1,
    HERO_AVAILABILITY_IN_TAVERN = 0x40
};

// m_fledState: how the hero left the map today. A hero rehired the day he
// retreated or surrendered has no movement left (or only his leftover
// movement with the SoftRetreatSurrender option); a surrendered hero keeps
// his army. Every new day clears it.
enum HeroFledState {
    HERO_FLED_NONE = 0,
    HERO_FLED_RETREATED = 1,
    HERO_FLED_SURRENDERED = 2
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
    HERO_PRIMARY_SIEGE = 4
};

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
    u8 m_locationMetadata;
    i16 m_mobility;
    i16 m_remainingMobility;
    i32 m_experience;
    i8 m_unused2d;
    i16 m_level;
    i8 m_primaryStats[HERO_STARTING_STAT_COUNT];
    i8 m_morale;
    i8 m_luck;
    i8 m_cowardice;
    i8 m_fledState;
    i32 m_visitedSites;
    i16 m_randomSeed;
    char m_unused3f[0x18];
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
    void GetArmyStrengths(u32* const);
    i8 HasArtifact(i8 artifact);
    i16 CalcMobility(void);
    i8 HasSpell(i8 spell);
    i16 GetNumSpells(i8 type);
    void UseSpell(i8 spell);
    i32 AddSpell(i8 spell, i8 charges, b32 checkOnly);
    void HeroScreenUpdate(void);
    void UpdateArmies(void);
    void RedrawHeroScreen(void);
    i8 HeroView(b8 viewOnly);
    void ViewStat(i8 stat, i8 quickView);
    void ViewArtifact(i8 artifact, i8 quickView);
    i8 Dismiss(void);
    void Deallocate(void);
    i32 GetExperience(i32 level);
    i32 GetLevel(i32 experienceValue);
    void ApplyBattleWinTemps(void);
    void ApplyBattleLossTemps(void);
    void ClearBattleTemps(void);
    void ResetToStartingState(void);
    void SetRecruitedMobility(void);
    void CheckLevel(void);
    i32 NumArtifacts(void);
};

extern class heroWindow* gHeroWin;

void HeroMessageUpdate(char* text);
void UpdateHeroScreenStatusBar(i16 widgetId);
extern i8 gHeroScreenActive;
i16 HeroHandler(struct tag_message& message);
enum HeroScreenControl {
    HERO_SCREEN_TITLE = 2,
    HERO_SCREEN_ARTIFACT_BACKGROUND_FIRST = 6,
    HERO_SCREEN_ARTIFACT_FIRST = 20,
    HERO_SCREEN_PORTRAIT = 65,
    HERO_SCREEN_STAT_VALUE_FIRST = 76,
    HERO_SCREEN_ATTACK = 81,
    HERO_SCREEN_STAT_FIRST = HERO_SCREEN_ATTACK,
    HERO_SCREEN_DEFENSE = 82,
    HERO_SCREEN_SPELL_POWER = 83,
    HERO_SCREEN_KNOWLEDGE = 84,
    HERO_SCREEN_CHARACTERISTICS = 85,
    HERO_SCREEN_CREST = 86,
    HERO_SCREEN_ARMY_BACKGROUND_FIRST = 87,
    HERO_SCREEN_ARMY_CREATURE_FIRST = 92,
    HERO_SCREEN_ARMY_COUNT_FIRST = 97,
    HERO_SCREEN_ARMY_SLOT_FIRST = 102,
    HERO_SCREEN_MORALE_FIRST = 200,
    HERO_SCREEN_MORALE_LAST = 202,
    HERO_SCREEN_LUCK_FIRST = 203,
    HERO_SCREEN_LUCK_LAST = 205,
    HERO_SCREEN_EXPERIENCE_ICON = 206,
    HERO_SCREEN_EXPERIENCE = 207,
    HERO_SCREEN_STATUS_FIRST = 300,
    HERO_SCREEN_STATUS_TEXT = 302,
    HERO_SCREEN_EXIT = DIALOG_BUTTON_0,
    HERO_SCREEN_DISMISS = DIALOG_BUTTON_3
};

enum HeroScreenText {
    HERO_TEXT_KINGDOM_OVERVIEW = 0,
    HERO_TEXT_PRIMARY_STAT = 1,
    HERO_TEXT_ADDITIONAL_STATS = 2,
    HERO_TEXT_GOOD_MORALE = 3,
    HERO_TEXT_NEUTRAL_MORALE = 4,
    HERO_TEXT_BAD_MORALE = 5,
    HERO_TEXT_GOOD_LUCK = 6,
    HERO_TEXT_NEUTRAL_LUCK = 7,
    HERO_TEXT_BAD_LUCK = 8,
    HERO_TEXT_EXPERIENCE = 9,
    HERO_TEXT_SELECT_ARMY = 10,
    HERO_TEXT_EMPTY = 11,
    HERO_TEXT_MOVE_ARMY = 12,
    HERO_TEXT_EXCHANGE_ARMIES = 13,
    HERO_TEXT_VIEW_SPELLS = 14,
    HERO_TEXT_ARTIFACT = 15,
    HERO_TEXT_DISMISS = 16,
    HERO_TEXT_EXIT = 17,
    HERO_TEXT_SCREEN = 18,
    HERO_TEXT_COUNT = 19
};

enum HeroScreenMoodFrame {
    HERO_LUCK_FRAME_GOOD = 11,
    HERO_LUCK_FRAME_BAD = 12,
    HERO_MORALE_FRAME_GOOD = 13,
    HERO_MORALE_FRAME_BAD = 14,
    HERO_LUCK_FRAME_NEUTRAL = 16,
    HERO_MORALE_FRAME_NEUTRAL = 17
};

enum HeroScreenMoodConstant {
    HERO_SCREEN_MOOD_ICON_COUNT = 3
};

enum HeroScreenArmyConstant {
    HERO_SCREEN_SOURCE_NONE = -1,
    HERO_ARMY_BACKGROUND_EMPTY_FRAME = 2,
    HERO_ARMY_BACKGROUND_FACTION_FIRST_FRAME = 3
};

enum HeroStatViewControl {
    HERO_STAT_VIEW_TITLE = 1,
    HERO_STAT_VIEW_DESCRIPTION = 2
};

enum HeroLevelText {
    HERO_LEVEL_TEXT_GAINED = 0,
    HERO_LEVEL_TEXT_ONE_LEVEL = 1,
    HERO_LEVEL_TEXT_LEVELS = 2,
    HERO_LEVEL_TEXT_COUNT = 3
};

enum HeroSpellPowerLevel {
    HERO_SPELL_POWER_ONE = 1,
    HERO_SPELL_POWER_TWO = 2
};

#endif

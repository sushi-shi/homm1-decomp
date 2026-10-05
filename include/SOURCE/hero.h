#ifndef HOMM1_SOURCE_HERO_H
#define HOMM1_SOURCE_HERO_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 34 methods, 0 own-virtual, 0 static data.

#include <BASE/dialog.h>
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
// (0x00493028) and their help texts; army::Init adds 0 and 1 to the
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
    i8 m_id;
    i8 m_owner;
    char m_name[0x11];
    // UpdBottomViewHero copies this 8-character label into its name widget.
    char m_shortName[9];
    // Indexes gClassNames, gMinExpForLevel and the class crest frames;
    // combat copies it to m_heroType.
    i8 m_heroClass;
    // port%04d.icn portrait number.
    i8 m_portrait;
    i8 m_x;
    i8 m_y;
    i8 m_destinationX;
    i8 m_destinationY;
    u8 m_direction;
    u8 m_locationType;
    // SetHeroContext passes it zero-extended to game::RestoreCell.
    u8 m_occupiedTown;
    i16 m_mobility;
    i16 m_remainingMobility;
    i32 m_experience;
    i8 m_unused2d;
    i16 m_level;
    // GiveTakeArtifactStat raises a fifth stat byte for artifact 17.
    i8 m_primaryStats[HERO_STARTING_STAT_COUNT];
    i8 m_morale;
    i8 m_luck;
    // ShowMoraleInfo reports the cowardice byte separately.
    i8 m_cowardice;
    i8 m_unused38;
    // DoAIEvent tests and sets one bit per visited site index.
    i32 m_visitedSites;
    i16 m_randomSeed;
    char m_unused3f[0x18];
    armyGroup m_army;
    // Combat spells fill the first 19 slots, adventure spells the last 10;
    // each memorized spell keeps its remaining casts in the parallel array.
    H1_ENUM_STORAGE(SpellType, i8) m_spells[HERO_SPELL_SLOT_COUNT];
    i8 m_spellCharges[HERO_SPELL_SLOT_COUNT];
    H1_ENUM_STORAGE(ArtifactType, i8) m_artifacts[HERO_ARTIFACT_SLOT_COUNT];
    i32 m_eventFlags;
    float m_aiFightValue;
    i32 IsEmbarked(void) {
        return m_eventFlags & HERO_EVENT_EMBARKED;
    }
    // --- constructors ---
    hero(void);
    // --- methods ---
    void Read(i32 file, i8 expansion);
    void Write(i32 file, i8 expansion);
    void GetArmyStrengths(u32* const);
    i8 HasArtifact(H1_ENUM_PARAM(ArtifactType, i8) artifact);
    i16 CalcMobility(void);
    i8 HasSpell(H1_ENUM_PARAM(SpellType, i8) spell);
    i32 GetNthSpell(i32 type, i32 spellNumber);
    i16 GetNumSpells(H1_ENUM_PARAM(HeroSpellType, i8) type);
    void UseSpell(H1_ENUM_PARAM(SpellType, i8) spell);
    i32 AddSpell(H1_ENUM_PARAM(SpellType, i8) spell, i8 charges, i32 checkOnly);
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
// Stale alias of gHeroWindShowing (0x494128): unreferenced, kept so later symbol handles stay put.
extern i8 gbHeroScreenActive;
i16 HeroHandler(struct tag_message& message);
// Moved from HERO.cpp.
// clang-format off
// herowind.bin widget ids. Names follow UpdateHeroScreenStatusBar's
// gHeroScreen texts (0x004937b0) and what HeroView, UpdateArmies and
// HeroHandler send to or do with each id; artifact and army slots are
// indexed from their first id, primary stats by HeroPrimaryStat.
H1_ENUM_BEGIN(HeroScreenControl)
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
H1_ENUM_END(HeroScreenControl)

// gHeroScreen (0x004937b0) status-bar texts, as UpdateHeroScreenStatusBar
// picks them: "Kingdom Overview", "View %s Info", "Additional hero
// characteristics", good/neutral/bad morale and luck, "View Experience
// Info", "Select %s", "Empty", "Move %s", "Exchange %s with %s", "View
// Spells", "View %s Info", "Dismiss %s the %s", "Exit Hero Screen",
// "Hero Screen" (Buka HeroScreenText numbering from 1).
H1_ENUM_BEGIN(HeroScreenText)
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
    HERO_TEXT_SCREEN = 18
H1_ENUM_END(HeroScreenText)

// Frames HeroView sets on the three luck and three morale icons, and how
// many of each the screen shows.
H1_ENUM_BEGIN(HeroScreenMoodFrame)
    HERO_LUCK_FRAME_GOOD = 11,
    HERO_LUCK_FRAME_BAD = 12,
    HERO_MORALE_FRAME_GOOD = 13,
    HERO_MORALE_FRAME_BAD = 14,
    HERO_LUCK_FRAME_NEUTRAL = 16,
    HERO_MORALE_FRAME_NEUTRAL = 17
H1_ENUM_END(HeroScreenMoodFrame)

H1_ENUM_CONST_BEGIN(HeroScreenMoodConstant)
    HERO_SCREEN_MOOD_ICON_COUNT = 3
H1_ENUM_CONST_END(HeroScreenMoodConstant)

// giHeroScreenSrcIndex: the army slot picked up on the hero screen, or NONE.
// UpdateArmies shows an empty slot with background frame EMPTY and a
// creature over its faction's frame (FACTION_FIRST + type / faction size).
H1_ENUM_CONST_BEGIN(HeroScreenArmyConstant)
    HERO_SCREEN_SOURCE_NONE = -1,
    HERO_ARMY_BACKGROUND_EMPTY_FRAME = 2,
    HERO_ARMY_BACKGROUND_FACTION_FIRST_FRAME = 3
H1_ENUM_CONST_END(HeroScreenArmyConstant)

// vstat.bin, ViewStat's primary-stat window: title and description texts.
H1_ENUM_BEGIN(HeroStatViewControl)
    HERO_STAT_VIEW_TITLE = 1,
    HERO_STAT_VIEW_DESCRIPTION = 2
H1_ENUM_END(HeroStatViewControl)

// gHeroLevel: CheckLevel's "%s has gained" lead, then one level or %d levels.
H1_ENUM_BEGIN(HeroLevelText)
    HERO_LEVEL_TEXT_GAINED = 0,
    HERO_LEVEL_TEXT_ONE_LEVEL = 1,
    HERO_LEVEL_TEXT_LEVELS = 2
H1_ENUM_END(HeroLevelText)

// Spell-power levels the AI weighs specially (philAI army worth, combat
// spell effect shift).
H1_ENUM_CONST_BEGIN(HeroSpellPowerLevel)
    HERO_SPELL_POWER_ONE = 1,
    HERO_SPELL_POWER_TWO = 2
H1_ENUM_CONST_END(HeroSpellPowerLevel)

#endif // HOMM1_SOURCE_HERO_H

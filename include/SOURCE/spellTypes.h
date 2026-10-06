#ifndef HOMM1_SOURCE_SPELLTYPES_H
#define HOMM1_SOURCE_SPELLTYPES_H

#include <Domains.h>

// HoMM1 spell ids: the order of retail gSpellNames (0x00493148), which
// combatManager::SpellMessage and the spell shrine print by id. The combat
// arms agree: ValidSpellTarget sends 1 to enemies, 4 to friends, 11 to
// ghosts and 0/17 to area hexes; CastSpell reports 1's lightning damage and
// 4's resurrections and dispatches 0, 15, 16 and 17 to Fireball,
// Armageddon, ElementalStorm and MeteorShower. hero::AddSpell keeps combat
// spells below 19 and marks an empty spell slot with -1.
H1_ENUM_BEGIN(SpellType)
    SPELL_NONE = -1,
    SPELL_FIREBALL = 0,
    SPELL_LIGHTNING_BOLT = 1,
    SPELL_TELEPORT = 2,
    SPELL_CURE = 3,
    SPELL_RESURRECT = 4,
    SPELL_HASTE = 5,
    SPELL_SLOW = 6,
    SPELL_BLIND = 7,
    SPELL_BLESS = 8,
    SPELL_PROTECTION = 9,
    SPELL_CURSE = 10,
    SPELL_TURN_UNDEAD = 11,
    SPELL_ANTI_MAGIC = 12,
    SPELL_DISPEL_MAGIC = 13,
    SPELL_BERZERKER = 14,
    SPELL_ARMAGEDDON = 15,
    SPELL_STORM = 16,
    SPELL_METEOR_SHOWER = 17,
    SPELL_PARALYZE = 18,
    SPELL_VIEW_MINES = 19,
    SPELL_VIEW_RESOURCES = 20,
    SPELL_VIEW_ARTIFACTS = 21,
    SPELL_VIEW_TOWNS = 22,
    SPELL_VIEW_HEROES = 23,
    SPELL_VIEW_ALL = 24,
    SPELL_IDENTIFY_HERO = 25,
    SPELL_SUMMON_BOAT = 26,
    SPELL_DIMENSION_DOOR = 27,
    SPELL_TOWN_GATE = 28,
    SPELL_COUNT = 29
H1_ENUM_END(SpellType)

// gSpellAIFlags bits: the AI casts COMBAT spells in battle (rows 0..18) and
// values ADVENTURE spells (19..28) on the map. SCALES_WITH_POWER marks
// fireball, lightning, resurrect, armageddon, storm and meteor shower, whose
// value philAI scales by the hero's spell power (CastSpell scores) or
// knowledge (mage guild and shrine values) and SetupTown weights four times.
H1_ENUM_FLAGS_BEGIN(SpellAIFlag, i32)
    SPELL_AI_FLAG_SCALES_WITH_POWER = 0x01,
    SPELL_AI_FLAG_COMBAT = 0x02,
    SPELL_AI_FLAG_ADVENTURE = 0x04
H1_ENUM_FLAGS_END(SpellAIFlag)

// gSpellHelp rows: the spell book's hover/right-click texts (CombatSpecialHandler
// in combat, game::ViewSpells on the map).
H1_ENUM_BEGIN(SpellHelpText)
    SPELL_HELP_PREVIOUS_PAGE = 0,
    SPELL_HELP_NEXT_PAGE = 1,
    SPELL_HELP_ADVENTURE_SPELLS = 2,
    SPELL_HELP_COMBAT_SPELLS = 3,
    SPELL_HELP_CLOSE = 4,
    SPELL_HELP_VIEW_SPELLS = 5,
    SPELL_HELP_SELECT_SPELL = 6,
    SPELL_HELP_VIEW_COMBAT_SPELLS = 7,
    SPELL_HELP_COUNT = 8
H1_ENUM_END(SpellHelpText)

#endif // HOMM1_SOURCE_SPELLTYPES_H

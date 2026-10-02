#ifndef HOMM1_SOURCE_SPELLTYPES_H
#define HOMM1_SOURCE_SPELLTYPES_H

#include <Domains.h>

// clang-format off
// HoMM1 spell ids: the order of retail gSpellNames (0x00493330), which
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
    SPELL_TOWN_GATE = 28
H1_ENUM_END(SpellType)
// clang-format on

#endif // HOMM1_SOURCE_SPELLTYPES_H

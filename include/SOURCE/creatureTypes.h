#ifndef HOMM1_SOURCE_CREATURETYPES_H
#define HOMM1_SOURCE_CREATURETYPES_H

#include <Domains.h>

// HoMM1 creature order: six per faction, then the four neutral types.
// armyGroup and army slots mark an empty stack with -1.
H1_ENUM_BEGIN(CreatureType)
    CREATURE_NONE = -1,
    CREATURE_PEASANT = 0,
    CREATURE_ARCHER = 1,
    CREATURE_PIKEMAN = 2,
    CREATURE_SWORDSMAN = 3,
    CREATURE_CAVALRY = 4,
    CREATURE_PALADIN = 5,
    CREATURE_GOBLIN = 6,
    CREATURE_ORC = 7,
    CREATURE_WOLF = 8,
    CREATURE_OGRE = 9,
    CREATURE_TROLL = 10,
    CREATURE_CYCLOPS = 11,
    CREATURE_SPRITE = 12,
    CREATURE_DWARF = 13,
    CREATURE_ELF = 14,
    CREATURE_DRUID = 15,
    CREATURE_UNICORN = 16,
    CREATURE_PHOENIX = 17,
    CREATURE_CENTAUR = 18,
    CREATURE_GARGOYLE = 19,
    CREATURE_GRIFFIN = 20,
    CREATURE_MINOTAUR = 21,
    CREATURE_HYDRA = 22,
    CREATURE_DRAGON = 23,
    CREATURE_ROGUE = 24,
    CREATURE_NOMAD = 25,
    CREATURE_GHOST = 26,
    CREATURE_GENIE = 27,
    CREATURE_COUNT = 28
H1_ENUM_END(CreatureType)

// clang-format off
// Creature attribute bits (monster record / army::m_stats.attributes), Buka
// 2.1 KB_TYPES.h MonsterFlags numbering: wide stacks take two hexes, flyers
// skip the path, shooters spend shots, breath attacks hit the hex behind;
// DEAD, HIGH_MORALE (a good-morale extra move), RETALIATED and TURN_SPENT are
// combat state. ResetRound keeps ROUND_PERSISTENT_MASK each round; GenerateMap
// keeps BATTLE_START_MASK when stacks enter the field.
H1_ENUM_FLAGS_BEGIN(MonsterFlags, int)
    MONSTER_FLAGS_WIDE = 0x01,
    MONSTER_FLAGS_FLYING = 0x02,
    MONSTER_FLAGS_SHOOTER = 0x04,
    MONSTER_FLAGS_BREATH_ATTACK = 0x08,
    MONSTER_FLAGS_DEAD = 0x10,
    MONSTER_FLAGS_HIGH_MORALE = 0x20,
    MONSTER_FLAGS_RETALIATED = 0x40,
    MONSTER_FLAGS_TURN_SPENT = 0x80,
    MONSTER_FLAGS_ROUND_PERSISTENT_MASK = 0x1f,
    MONSTER_FLAGS_BATTLE_START_MASK = 0x3f
H1_ENUM_FLAGS_END(MonsterFlags)
// clang-format on

#endif

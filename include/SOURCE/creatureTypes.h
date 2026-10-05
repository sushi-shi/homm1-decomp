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

// tag_monsterStats::speed, indexing gSpeedText ("", Slow, Medium, Fast,
// Blazing). Slow sets SLOW, haste BLAZING, blind NONE; combat rounds count
// m_currentSpeed down from BLAZING.
H1_ENUM_BEGIN(CreatureSpeed)
    CREATURE_SPEED_NONE = 0,
    CREATURE_SPEED_SLOW = 1,
    // Half-open ends of the speed bands the spell AI weighs haste by
    // (speed < SLOW_END: slow or stopped; speed < MEDIUM_END: at most medium).
    CREATURE_SPEED_SLOW_END = 2,
    CREATURE_SPEED_MEDIUM = 2,
    CREATURE_SPEED_MEDIUM_END = 3,
    CREATURE_SPEED_FAST = 3,
    CREATURE_SPEED_BLAZING = 4
H1_ENUM_END(CreatureSpeed)

// Each race's six creatures are consecutive: creature / FACTION_SIZE is the
// race (philAI's same-race bonus, KB's army alignment test).
H1_ENUM_CONST_BEGIN(CreatureFactionConstant)
    CREATURE_FACTION_SIZE = 6
H1_ENUM_CONST_END(CreatureFactionConstant)

// Creature attribute bits (monster record / army::m_stats.attributes): wide
// stacks take two hexes, flyers skip the path, shooters spend shots, breath attacks hit the hex behind;
// DEAD, HIGH_MORALE (a good-morale extra move), RETALIATED and TURN_SPENT are
// combat state. ResetRound keeps ROUND_PERSISTENT_MASK each round; GenerateMap
// keeps BATTLE_START_MASK when stacks enter the field.
H1_ENUM_FLAGS_BEGIN(MonsterFlags, i32)
    MONSTER_FLAGS_NONE = 0x00,
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

#pragma pack(push, 1)
// HoMM1 monster records are 0x1f bytes: GetMonsterCost reads the cost word at
// +0, retail readers use a dword at +8 and test attribute bits at +0x1b.
// army::Init copies these 0x13 bytes from record +0xc into each combat stack.
struct tag_monsterStats {
    // army::Resurrect divides by this byte zero-extended.
    u8 hitPoints;
    i8 speed;
    i8 missileType;
    i8 attack;
    i8 defense;
    i8 damageMin;
    i8 damageMax;
    // army::PowEffect index into gPowEffectNames.
    i8 powEffect;
    i8 shots;
    char unknown09[6];
    i32 attributes;
};
struct tag_monsterInfo {
    i16 cost;
    i32 fightValue;
    i8 iconIndex;
    i8 growth;
    i32 hitPoints;
    tag_monsterStats stats;
};
#pragma pack(pop)

#endif

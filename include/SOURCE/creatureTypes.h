#ifndef HOMM1_SOURCE_CREATURETYPES_H
#define HOMM1_SOURCE_CREATURETYPES_H

enum CreatureType {
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
};

enum CreatureSpeed {
    CREATURE_SPEED_NONE = 0,
    CREATURE_SPEED_SLOW = 1,
    CREATURE_SPEED_SLOW_END = 2,
    CREATURE_SPEED_MEDIUM = 2,
    CREATURE_SPEED_MEDIUM_END = 3,
    CREATURE_SPEED_FAST = 3,
    CREATURE_SPEED_BLAZING = 4
};

enum CreatureFactionConstant {
    CREATURE_FACTION_SIZE = 6
};

enum MonsterFlags {
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
};

#pragma pack(push, 1)
struct tag_monsterStats {
    u8 hitPoints;
    i8 speed;
    i8 missileType;
    i8 attack;
    i8 defense;
    i8 damageMin;
    i8 damageMax;
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

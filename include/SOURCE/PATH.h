#ifndef HOMM1_SOURCE_PATH_H
#define HOMM1_SOURCE_PATH_H

#include <SOURCE/combatTypes.h>

enum ArmyAttackTarget {
    ARMY_ATTACK_TARGET_ASSIGNED = 0,
    ARMY_ATTACK_TARGET_ENEMY = 1,
    ARMY_ATTACK_TARGET_OCCUPIED = 2
};

enum ArmyPathTarget {
    ARMY_PATH_ASSIGNED_TARGET_HEX = -1,
    ARMY_PATH_ANY_TARGET_HEX = 0,
    ARMY_PATH_EXACT_TARGET_HEX = 1
};
#define ARMY_PATH_ATTACKS(mode) (mode)

enum ArmyHexConstant {
    ARMY_HEX_INVALID = -1
};

i16
OppositeDirection(i16 direction);

enum CombatPathConstant {
    WIDE_DIRECTIONS_MASK = 0xc0,
    IGNORE_SPEED = 99,
    WIDE_HEX_OFFSET = 1
};

#endif

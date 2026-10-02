#ifndef HOMM1_SOURCE_PATH_H
#define HOMM1_SOURCE_PATH_H

#include <SOURCE/combatTypes.h>

// clang-format off
// army::ValidAttack / GetAttackMask targetMode: the assigned target stack
// (m_targetSide/m_targetIndex), any enemy stack, or any occupied hex (Buka
// army.h ArmyAttackTarget, same numbering and switch).
H1_ENUM_BEGIN(ArmyAttackTarget)
    ARMY_ATTACK_TARGET_ASSIGNED = 0,
    ARMY_ATTACK_TARGET_ENEMY = 1,
    ARMY_ATTACK_TARGET_OCCUPIED = 2
H1_ENUM_END(ArmyAttackTarget)

// A hex argument or result meaning "no hex": GetAdjacentCellIndex's
// off-grid result and ValidAttack's "any target hex" (Buka ArmyHexConstant).
H1_ENUM_CONST_BEGIN(ArmyHexConstant)
    ARMY_HEX_INVALID = -1
H1_ENUM_CONST_END(ArmyHexConstant)
// clang-format on

H1_ENUM_RETURN(CombatHexDirection, short)
OppositeDirection(H1_ENUM_PARAM(CombatHexDirection, short) direction);

#endif

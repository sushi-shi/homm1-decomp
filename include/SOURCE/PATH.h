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

// army::FindPath/ValidPath/ValidFlight path mode, forwarded to
// searchArray::FindCombatPath's attackPath: nonzero (with an assigned target)
// stops at the first hex from which the target can be attacked instead of
// routing onto it. The combat AI passes EXACT walking to a stack's front and
// ASSIGNED closing on its target (Buka combatTypes.h ArmyPathTarget
// numbering; HoMM1 treats every nonzero mode alike).
H1_ENUM_BEGIN(ArmyPathTarget)
    ARMY_PATH_ASSIGNED_TARGET_HEX = -1,
    ARMY_PATH_ANY_TARGET_HEX = 0,
    ARMY_PATH_EXACT_TARGET_HEX = 1
H1_ENUM_END(ArmyPathTarget)

// A hex argument or result meaning "no hex": GetAdjacentCellIndex's
// off-grid result and ValidAttack's "any target hex" (Buka ArmyHexConstant).
H1_ENUM_CONST_BEGIN(ArmyHexConstant)
    ARMY_HEX_INVALID = -1
H1_ENUM_CONST_END(ArmyHexConstant)
// clang-format on

H1_ENUM_RETURN(CombatHexDirection, short)
OppositeDirection(H1_ENUM_PARAM(CombatHexDirection, short) direction);

#endif

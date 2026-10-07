#ifndef HOMM1_SOURCE_PATH_H
#define HOMM1_SOURCE_PATH_H

#include <Domains.h>
#include <SOURCE/combatTypes.h>

// army::ValidAttack / GetAttackMask targetMode: the assigned target stack
// (m_targetSide/m_targetIndex), any enemy stack, or any occupied hex.
H1_ENUM_BEGIN(ArmyAttackTarget)
    ARMY_ATTACK_TARGET_ASSIGNED = 0,
    ARMY_ATTACK_TARGET_ENEMY = 1,
    ARMY_ATTACK_TARGET_OCCUPIED = 2
H1_ENUM_END(ArmyAttackTarget)

// army::FindPath/ValidPath/ValidFlight path mode, forwarded to
// searchArray::FindCombatPath's attackPath: nonzero passes the target hex to
// GetAttackMask and stops at the first hex from which the target can be
// attacked instead of routing onto it; ANY (0) passes ARMY_HEX_INVALID.
// COMMAND moves pass ANY and attack routes EXACT; the combat AI passes EXACT
// walking to a stack's front and ASSIGNED closing on its target (every
// nonzero mode is treated alike).
H1_ENUM_BEGIN(ArmyPathTarget)
    ARMY_PATH_ASSIGNED_TARGET_HEX = -1,
    ARMY_PATH_ANY_TARGET_HEX = 0,
    ARMY_PATH_EXACT_TARGET_HEX = 1
H1_ENUM_END(ArmyPathTarget)
// The mode routes to a hex from which the target can be attacked (every
// mode but ANY).
#if H1_STRICT_DOMAINS
inline constexpr bool ArmyPathAttacks(ArmyPathTarget mode) {
    return mode != ARMY_PATH_ANY_TARGET_HEX;
}
#define ARMY_PATH_ATTACKS(mode) ArmyPathAttacks(mode)
#else
#define ARMY_PATH_ATTACKS(mode) (mode)
#endif

// A hex argument or result meaning "no hex": GetAdjacentCellIndex's
// off-grid result and ValidAttack's "any target hex".
H1_ENUM_CONST_BEGIN(ArmyHexConstant)
    ARMY_HEX_INVALID = -1
H1_ENUM_CONST_END(ArmyHexConstant)

H1_ENUM_RETURN(CombatHexDirection, i16)
OppositeDirection(H1_ENUM_PARAM(CombatHexDirection, i16) direction);

// CombatPathConstant: the blocked-mask bits for the two
// wide-creature directions, the speed FindPath grants when speed is ignored,
// and the second hex of a wide creature.
H1_ENUM_CONST_BEGIN(CombatPathConstant)
    SPECIAL_DIRECTION_MASK = 0xc0,
    IGNORE_SPEED = 99,
    WIDE_HEX_OFFSET = 1
H1_ENUM_CONST_END(CombatPathConstant)

#endif

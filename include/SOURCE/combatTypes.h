#ifndef HOMM1_SOURCE_COMBATTYPES_H
#define HOMM1_SOURCE_COMBATTYPES_H

#include <Domains.h>

H1_ENUM_BEGIN(CombatHexDirection)
    COMBAT_DIRECTION_INVALID = -1,
    COMBAT_DIRECTION_NORTHEAST = 0,
    COMBAT_DIRECTION_EAST = 1,
    COMBAT_DIRECTION_SOUTHEAST = 2,
    COMBAT_DIRECTION_SOUTHWEST = 3,
    COMBAT_DIRECTION_WEST = 4,
    COMBAT_DIRECTION_NORTHWEST = 5,
    COMBAT_DIRECTION_WIDE_WEST = 6,
    COMBAT_DIRECTION_WIDE_EAST = 7,
    COMBAT_DIRECTION_OPPOSITE_OFFSET = 3,
    COMBAT_DIRECTION_ADJACENT_COUNT = 6,
    COMBAT_DIRECTION_COUNT = 8
H1_ENUM_END(CombatHexDirection)

// clang-format off
// combatManager's per-side arrays (m_armies, m_heroes, m_playerId, ...).
// HoMM1's SetupCombat stores the attacker in side 1 and the defender in side
// 0 (Buka 2.1 combatTypes.h CombatSide has the opposite numbering); -1 marks
// an empty hex or no target.
H1_ENUM_BEGIN(CombatSide)
    COMBAT_SIDE_NONE = -1,
    COMBAT_DEFENDER_SIDE = 0,
    COMBAT_ATTACKER_SIDE = 1,
    COMBAT_SIDE_COUNT = 2
H1_ENUM_END(CombatSide)

// army::m_facing, also passed as the sprite orientation: the attacker (side
// 1) starts at column 1 with facing side ^ 1 = 0, so 0 faces right and 1 is
// the mirrored, left-facing sprite (Buka ArmyFacing numbers them the other
// way round, with its attacker in side 0).
H1_ENUM_BEGIN(ArmyFacing)
    ARMY_FACING_RIGHT = 0,
    ARMY_FACING_LEFT = 1
H1_ENUM_END(ArmyFacing)
// clang-format on

H1_ENUM_BEGIN(CombatEffectDimension)
    COMBAT_EFFECT_SIDE_COUNT = 2,
    COMBAT_EFFECT_SLOT_COUNT = 5
H1_ENUM_END(CombatEffectDimension)

extern signed char gArmyEffected[COMBAT_EFFECT_SIDE_COUNT][COMBAT_EFFECT_SLOT_COUNT];

// HoMM1 spell-AI row traversal: retail NextPos divides by nine.
H1_ENUM_BEGIN(CombatSpellAIGrid)
    COMBAT_SPELL_AI_ROW_LENGTH = 9,
    COMBAT_SPELL_AI_ROW_END_OFFSET = 2,
    COMBAT_SPELL_AI_ROW_SKIP = 3
H1_ENUM_END(CombatSpellAIGrid)

#endif

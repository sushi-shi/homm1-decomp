#ifndef HOMM1_SOURCE_COMBATTYPES_H
#define HOMM1_SOURCE_COMBATTYPES_H

#include <Domains.h>
#include <SOURCE/armyGroup.h>

H1_ENUM_BEGIN(CombatHexDirection)
    COMBAT_DIRECTION_INVALID = -1,
    COMBAT_DIRECTION_FIRST = 0,
    COMBAT_DIRECTION_NORTHEAST = 0,
    COMBAT_DIRECTION_EAST = 1,
    COMBAT_DIRECTION_SOUTHEAST = 2,
    COMBAT_DIRECTION_SOUTHWEST = 3,
    COMBAT_DIRECTION_WEST = 4,
    COMBAT_DIRECTION_NORTHWEST = 5,
    COMBAT_DIRECTION_ADJACENT_LAST = 5,
    COMBAT_DIRECTION_WIDE_WEST = 6,
    COMBAT_DIRECTION_WIDE_EAST = 7,
    COMBAT_DIRECTION_EASTERN_FIRST = 0,
    COMBAT_DIRECTION_EASTERN_LAST = 2,
    COMBAT_DIRECTION_WESTERN_FIRST = 3,
    COMBAT_DIRECTION_WESTERN_LAST = 5,
    COMBAT_DIRECTION_WIDE_FIRST = 6,
    COMBAT_DIRECTION_ADJACENT_FIRST = 0,
    COMBAT_DIRECTION_ADJACENT_COUNT = 6,
    COMBAT_DIRECTION_LAST = 7,
    COMBAT_DIRECTION_COUNT = 8
H1_ENUM_END(CombatHexDirection)
H1_ENUM_STEPPED(CombatHexDirection)
// The adjacent direction half way round the six.
#if H1_STRICT_DOMAINS
inline constexpr CombatHexDirection CombatOppositeAdjacent(CombatHexDirection direction);
#define COMBAT_OPPOSITE_ADJACENT(direction) CombatOppositeAdjacent(direction)
#else
#define COMBAT_OPPOSITE_ADJACENT(direction)                                                        \
    (((direction) + COMBAT_DIRECTION_OPPOSITE_OFFSET) % COMBAT_DIRECTION_ADJACENT_COUNT)
#endif

// OppositeDirection turns an adjacent direction half way round the six.
H1_ENUM_CONST_BEGIN(CombatDirectionConstant)
    COMBAT_DIRECTION_OPPOSITE_OFFSET = 3
H1_ENUM_CONST_END(CombatDirectionConstant)
#if H1_STRICT_DOMAINS
inline constexpr CombatHexDirection CombatOppositeAdjacent(CombatHexDirection direction) {
    return static_cast<CombatHexDirection>(
        (static_cast<int>(direction) + COMBAT_DIRECTION_OPPOSITE_OFFSET)
        % static_cast<int>(COMBAT_DIRECTION_ADJACENT_COUNT)
    );
}
#endif

// combatManager::m_directionMap: the attack pointer's 24 sectors around the
// target hex, four per neighbour direction; SetCombatDirections marks
// sectors it fills from a neighbour with +FILLED during a pass.
H1_ENUM_CONST_BEGIN(CombatPointerSectorConstant)
    COMBAT_POINTER_SECTOR_COUNT = 24,
    COMBAT_POINTER_SECTORS_PER_DIRECTION = 4,
    COMBAT_POINTER_SECTOR_FILLED = 10
H1_ENUM_CONST_END(CombatPointerSectorConstant)

// combatManager's per-side arrays (m_armies, m_heroes, m_playerId, ...).
// SetupCombat stores the attacker in side 1 and the defender in side 0; -1
// marks an empty hex or no target.
// A combat side. combatManager::m_combatResult holds the side that won
// (CheckWin; a retreating side loses to the other) and DoVictory indexes the
// per-side tables by it: DRAW (no side) when both sides fall, PENDING from
// Open until the battle ends; advManager::DoCombat switches on it for losses.
// ANY selects both sides: CastMassSpell's castSide (mass dispel) and the
// spell AI's FirstArmy/EffectSpellCure target side.
H1_ENUM_BEGIN(CombatSide)
    COMBAT_SIDE_NONE = -1,
    COMBAT_SIDE_FIRST = 0,
    COMBAT_DEFENDER_SIDE = 0,
    COMBAT_ATTACKER_SIDE = 1,
    COMBAT_SIDE_COUNT = 2,
    COMBAT_SIDE_ANY = COMBAT_SIDE_COUNT,
    COMBAT_RESULT_DRAW = COMBAT_SIDE_NONE,
    COMBAT_RESULT_DEFENDER = COMBAT_DEFENDER_SIDE,
    COMBAT_RESULT_ATTACKER = COMBAT_ATTACKER_SIDE,
    COMBAT_RESULT_PENDING = 3
H1_ENUM_END(CombatSide)
H1_ENUM_STEPPED(CombatSide)
// The other of the two sides.
#if H1_STRICT_DOMAINS
inline constexpr CombatSide CombatOpposingSide(CombatSide side) {
    return static_cast<CombatSide>(1 - static_cast<int>(side));
}
#define COMBAT_OPPOSING_SIDE(side) CombatOpposingSide(side)
#else
#define COMBAT_OPPOSING_SIDE(side) (1 - (side))
#endif
// Turn a side variable to the other side in place.
#if H1_STRICT_DOMAINS
inline CombatSide& CombatSwitchSide(CombatSide& side) {
    return side = static_cast<CombatSide>(static_cast<int>(side) ^ 1);
}
#define COMBAT_SWITCH_SIDE(side) CombatSwitchSide(side)
#else
#define COMBAT_SWITCH_SIDE(side) ((side) ^= 1)
#endif

// army::m_facing, also passed as the sprite orientation: the attacker (side
// 1) starts at column 1 with facing side ^ 1 = 0, so 0 faces right and 1 is
// the mirrored, left-facing sprite.
// hexcell::m_occupantFootprintHalf holds the facing-side half of a wide
// stack a cell shows, NONE for a one-hex stack or an empty cell.
H1_ENUM_BEGIN(ArmyFacing)
    ARMY_FACING_NONE = -1,
    ARMY_FACING_RIGHT = 0,
    ARMY_FACING_LEFT = 1
H1_ENUM_END(ArmyFacing)
// The step from a wide stack's hex to its second hex: west (-1) for a
// left-facing stack, east (+1) for a right-facing one.
#if H1_STRICT_DOMAINS
inline constexpr int ArmyWideHexStep(ArmyFacing facing) {
    return facing != ARMY_FACING_RIGHT ? -1 : 1;
}
#define ARMY_WIDE_HEX_STEP(facing) ArmyWideHexStep(facing)
#else
#define ARMY_WIDE_HEX_STEP(facing) ((facing) ? -1 : 1)
#endif

// GetMoveMask/GetAttackMask/GetBestDirection blocked-direction masks: bit n
// is CombatHexDirection n (GetBestDirection returns n when bit n is clear);
// GetAttackMask == ALL_BLOCKED means no stack can be attacked from here.
H1_ENUM_FLAGS_BEGIN(CombatDirectionMask, i16)
    COMBAT_DIRECTION_BIT_NORTHEAST = 0x01,
    COMBAT_DIRECTION_BIT_EAST = 0x02,
    COMBAT_DIRECTION_BIT_SOUTHEAST = 0x04,
    COMBAT_DIRECTION_BIT_SOUTHWEST = 0x08,
    COMBAT_DIRECTION_BIT_WEST = 0x10,
    COMBAT_DIRECTION_BIT_NORTHWEST = 0x20,
    COMBAT_DIRECTION_BIT_WIDE_WEST = 0x40,
    COMBAT_DIRECTION_BIT_WIDE_EAST = 0x80,
    COMBAT_ALL_DIRECTIONS_BLOCKED = 0xff
H1_ENUM_FLAGS_END(CombatDirectionMask)

// army::SpellEffect's animation: the gCombatFxNames row (rows 0..18 follow the
// combat SpellType order; 22..25 are rainbluk/cloudluk/moraleg/moraleb, played
// by army::CheckLuck and CheckApplyGood/BadMorale). CastSpell picks rows per
// spell, e.g. haste and blind reuse the slow row.
H1_ENUM_BEGIN(CombatEffectAnimation)
    COMBAT_EFFECT_NONE = -1,
    COMBAT_EFFECT_FIREBALL = 0,
    COMBAT_EFFECT_LIGHTNING_BOLT = 1,
    COMBAT_EFFECT_TELEPORT = 2,
    COMBAT_EFFECT_CURE = 3,
    COMBAT_EFFECT_RESURRECT = 4,
    COMBAT_EFFECT_HASTE = 5,
    COMBAT_EFFECT_SLOW = 6,
    COMBAT_EFFECT_BLIND = 7,
    COMBAT_EFFECT_BLESS = 8,
    COMBAT_EFFECT_PROTECTION = 9,
    COMBAT_EFFECT_CURSE = 10,
    COMBAT_EFFECT_TURN_UNDEAD = 11,
    COMBAT_EFFECT_ANTI_MAGIC = 12,
    COMBAT_EFFECT_DISPEL_MAGIC = 13,
    COMBAT_EFFECT_BERZERKER = 14,
    COMBAT_EFFECT_ARMAGEDDON = 15,
    COMBAT_EFFECT_STORM = 16,
    COMBAT_EFFECT_METEOR_SHOWER = 17,
    COMBAT_EFFECT_PARALYZE = 18,
    COMBAT_EFFECT_GOOD_LUCK = 22,
    COMBAT_EFFECT_BAD_LUCK = 23,
    COMBAT_EFFECT_GOOD_MORALE = 24,
    COMBAT_EFFECT_BAD_MORALE = 25,
    COMBAT_EFFECT_COUNT = 26
H1_ENUM_END(CombatEffectAnimation)

// hexcell::m_obstacleIndex: a rock's frame in the obstacle icon, or for a
// castle piece (column 5) the wall state. GenerateMap builds the wall INTACT;
// Catapult marks the struck piece HIT (from INTACT) or DAMAGED_HIT, then it
// either survives as DAMAGED or COLLAPSES and is cleared to NONE.
// hexcell::DrawObstacle draws INTACT/DAMAGED as those tower frames and the
// HIT states with DrawWall. Elsewhere GenerateMap picks a rock frame from
// FIRST_FRAME to LAND_ONLY_FRAME and redraws LAND_ONLY as FIRST on water and
// lava.
H1_ENUM_BEGIN(CombatObstacleIndex)
    COMBAT_OBSTACLE_NONE = -1,
    COMBAT_OBSTACLE_FIRST_FRAME = 0,
    COMBAT_OBSTACLE_LAND_ONLY_FRAME = 2,
    COMBAT_WALL_INTACT = 8,
    COMBAT_WALL_DAMAGED = 10,
    COMBAT_WALL_INTACT_HIT = 0x40,
    COMBAT_WALL_DAMAGED_HIT = 0x41,
    COMBAT_WALL_COLLAPSING = 0x42
H1_ENUM_END(CombatObstacleIndex)

// combatManager::m_combatIcons slots, as LoadCombatResources fills them:
// the terrain's ground and obstacle icons, textbar.icn, catapult.icn,
// tent.icn, castle%02d.icn, cloud.icn, keep%02d.icn and spells.icn.
// hexcell::m_groundIcon and m_obstacleIcon name the slot a cell draws from.
H1_ENUM_BEGIN(CombatIconSlot)
    COMBAT_ICON_GROUND = 0,
    COMBAT_ICON_TEXTBAR = 1,
    COMBAT_ICON_OBSTACLES = 2,
    COMBAT_ICON_CATAPULT = 3,
    COMBAT_ICON_TENT = 4,
    COMBAT_ICON_CASTLE = 5,
    COMBAT_ICON_CLOUD = 6,
    COMBAT_ICON_KEEP = 7,
    COMBAT_ICON_SPELLS = 8,
    COMBAT_ICON_COUNT = 9
H1_ENUM_END(CombatIconSlot)
H1_ENUM_STEPPED(CombatIconSlot)

// GetGridIndex hexes of the hero portraits beside the field: GetCommand and
// RightClick open the defender's (row 2, last column) or the attacker's
// (row 1, column 0) general.
H1_ENUM_CONST_BEGIN(CombatHeroHex)
    COMBAT_ATTACKER_HERO_HEX = 9,
    COMBAT_DEFENDER_HERO_HEX = 26
H1_ENUM_CONST_END(CombatHeroHex)

// gPowEffectNames rows army::PowEffect loads for the hit animation, named by
// their icon files (cloud.icn, physical.icn, redfire.icn, electric.icn).
// Creature records pick their own row; the damage spells use these: turn
// undead CLOUD, meteor shower PHYSICAL, fireball/lightning/armageddon
// RED_FIRE, storm ELECTRIC.
H1_ENUM_BEGIN(CombatPowEffect)
    COMBAT_POW_CLOUD = 0,
    COMBAT_POW_PHYSICAL = 1,
    COMBAT_POW_RED_FIRE = 7,
    COMBAT_POW_ELECTRIC = 8
H1_ENUM_END(CombatPowEffect)

// The combat field: hex = row * COLUMNS + column (GenerateMap, GetGridIndex);
// columns 0 and LAST_COLUMN are the castle/edge columns.
H1_ENUM_CONST_BEGIN(CombatGridDimension)
    COMBAT_GRID_COLUMNS = 9,
    COMBAT_GRID_ROWS = 5,
    COMBAT_GRID_LAST_COLUMN = 8,
    COMBAT_GRID_LAST_ROW = 4,
    // The standable columns lie between the edge columns (SetCombatDirections).
    COMBAT_GRID_FIRST_INNER_COLUMN = 1,
    COMBAT_GRID_LAST_INNER_COLUMN = 7
H1_ENUM_CONST_END(CombatGridDimension)

// HoMM1 spell-AI row traversal: retail NextPos steps along a row of
// COMBAT_GRID_COLUMNS hexes, skipping the two edge columns. A constant
// group: the offsets and bounds are hex-index arithmetic, not a value domain.
H1_ENUM_CONST_BEGIN(CombatSpellAIGrid)
    COMBAT_SPELL_AI_ROW_END_OFFSET = 2,
    COMBAT_SPELL_AI_ROW_SKIP = 3,
    // The scan runs over the field's inner hexes, from row 0 column 1 to
    // row 4 column 7 (DetermineEffectOfSpell, FirstArmy, EffectSpellDamage).
    COMBAT_SPELL_AI_HEX_FIRST = 1,
    COMBAT_SPELL_AI_HEX_LAST = 0x2b
H1_ENUM_CONST_END(CombatSpellAIGrid)

#endif

#ifndef HOMM1_SOURCE_COMBATTYPES_H
#define HOMM1_SOURCE_COMBATTYPES_H

#include <Domains.h>
#include <SOURCE/armyGroup.h>

H1_ENUM_BEGIN(CombatHexDirection)
    COMBAT_DIRECTION_INVALID = -1,
    COMBAT_DIRECTION_NORTHEAST = 0,
    COMBAT_DIRECTION_EAST = 1,
    COMBAT_DIRECTION_SOUTHEAST = 2,
    COMBAT_DIRECTION_SOUTHWEST = 3,
    COMBAT_DIRECTION_WEST = 4,
    COMBAT_DIRECTION_NORTHWEST = 5,
    COMBAT_DIRECTION_ADJACENT_LAST = 5,
    COMBAT_DIRECTION_WIDE_WEST = 6,
    COMBAT_DIRECTION_WIDE_EAST = 7,
    COMBAT_DIRECTION_EASTERN_LAST = 2,
    COMBAT_DIRECTION_WESTERN_FIRST = 3,
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

// GetMoveMask/GetAttackMask/GetBestDirection blocked-direction masks: bit n
// is CombatHexDirection n (GetBestDirection returns n when bit n is clear);
// GetAttackMask == ALL_BLOCKED means no stack can be attacked from here.
H1_ENUM_FLAGS_BEGIN(CombatDirectionMask, short)
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

// glTimers slot the combat screens pace their animation frames with
// (combatManager::Open/Main/KeepAttack, the win/lose windows); Buka keeps
// per-owner slot names the same way (HIGH_SCORE_TIMER_SLOT).
H1_ENUM_CONST_BEGIN(CombatTimerSlot)
    COMBAT_FRAME_TIMER_SLOT = 0
H1_ENUM_CONST_END(CombatTimerSlot)

// hexcell::m_obstacleIndex: a rock's frame in the obstacle icon, or for a
// castle piece (column 5) the wall state. GenerateMap builds the wall INTACT;
// Catapult marks the struck piece HIT (from INTACT) or DAMAGED_HIT, then it
// either survives as DAMAGED or COLLAPSES and is cleared to NONE.
// hexcell::DrawObstacle draws INTACT/DAMAGED as those tower frames and the
// HIT states with DrawWall.
H1_ENUM_BEGIN(CombatObstacleIndex)
    COMBAT_OBSTACLE_NONE = -1,
    COMBAT_WALL_INTACT = 8,
    COMBAT_WALL_DAMAGED = 10,
    COMBAT_WALL_INTACT_HIT = 0x40,
    COMBAT_WALL_DAMAGED_HIT = 0x41,
    COMBAT_WALL_COLLAPSING = 0x42
H1_ENUM_END(CombatObstacleIndex)

// GetGridIndex hexes of the hero portraits beside the field: GetCommand and
// RightClick open the defender's (row 2, last column) or the attacker's
// (row 1, column 0) general.
H1_ENUM_CONST_BEGIN(CombatHeroHex)
    COMBAT_ATTACKER_HERO_HEX = 9,
    COMBAT_DEFENDER_HERO_HEX = 26
H1_ENUM_CONST_END(CombatHeroHex)

// combatManager::m_combatResult, the side that won (CheckWin; a retreating
// side loses to the other), DRAW when both sides fall, PENDING from Open
// until the battle ends; advManager::DoCombat switches on it for losses
// (Buka CombatResult names, HoMM1 side numbering).
H1_ENUM_BEGIN(CombatResult)
    COMBAT_RESULT_DRAW = -1,
    COMBAT_RESULT_DEFENDER = 0,
    COMBAT_RESULT_ATTACKER = 1,
    COMBAT_RESULT_PENDING = 3
H1_ENUM_END(CombatResult)

// A side argument meaning both sides: CastMassSpell's castSide (mass dispel)
// and the spell AI's FirstArmy/EffectSpellCure target side (Buka SPELLAI
// SPELL_AI_ANY_SIDE).
H1_ENUM_CONST_BEGIN(CombatSideSelection)
    COMBAT_SIDE_ANY = 2
H1_ENUM_CONST_END(CombatSideSelection)

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
    COMBAT_GRID_LAST_ROW = 4
H1_ENUM_CONST_END(CombatGridDimension)
// clang-format on

// Area spells mark each stack once per cast: [side][army slot].
extern signed char gArmyEffected[COMBAT_SIDE_COUNT][ARMY_GROUP_SLOT_COUNT];

// HoMM1 spell-AI row traversal: retail NextPos steps along a row of
// COMBAT_GRID_COLUMNS hexes, skipping the two edge columns.
H1_ENUM_BEGIN(CombatSpellAIGrid)
    COMBAT_SPELL_AI_ROW_END_OFFSET = 2,
    COMBAT_SPELL_AI_ROW_SKIP = 3
H1_ENUM_END(CombatSpellAIGrid)

#endif

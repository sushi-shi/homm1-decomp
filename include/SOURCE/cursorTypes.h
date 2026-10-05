#ifndef HOMM1_SOURCE_CURSORTYPES_H
#define HOMM1_SOURCE_CURSORTYPES_H

#include <Domains.h>

H1_ENUM_BEGIN(MapDirection)
    MAP_DIRECTION_NORTH = 0,
    MAP_DIRECTION_NORTH_EAST = 1,
    MAP_DIRECTION_EAST = 2,
    MAP_DIRECTION_SOUTH_EAST = 3,
    MAP_DIRECTION_SOUTH = 4,
    MAP_DIRECTION_SOUTH_WEST = 5,
    MAP_DIRECTION_WEST = 6,
    MAP_DIRECTION_NORTH_WEST = 7,
    MAP_DIRECTION_COUNT = 8,
    // Cursor/hero directions after this one use the mirrored frames
    // (advManager::CompleteDraw's redraw order).
    MAP_DIRECTION_UNMIRRORED_LAST = MAP_DIRECTION_SOUTH
H1_ENUM_END(MapDirection)

// Direction bit masks over MapDirection (1 << direction): a step north-west,
// north or north-east is blocked by an object on the cell it leaves, a step
// south-east, south or south-west by one on the cell it enters (the hero
// cursor's move test and the path searches).
H1_ENUM_CONST_BEGIN(MapDirectionMask)
    MAP_DIRECTION_NORTH_MASK = 0x83,
    MAP_DIRECTION_SOUTH_MASK = 0x38
H1_ENUM_CONST_END(MapDirectionMask)

// gConfig.walkSpeed ("Walk Speed"): the Speed menu's Walk..Jump commands
// store 0..4; advManager's hero walk indexes gStepDelay by it and skips
// frames and sounds at JUMP.
H1_ENUM_BEGIN(WalkSpeed)
    WALK_SPEED_WALK = 0,
    WALK_SPEED_FIRST = WALK_SPEED_WALK,
    WALK_SPEED_TROT = 1,
    WALK_SPEED_CANTER = 2,
    WALK_SPEED_GALLOP = 3,
    WALK_SPEED_JUMP = 4,
    WALK_SPEED_COUNT = 5
H1_ENUM_END(WalkSpeed)

// The opposite direction is (d + OPPOSITE_OFFSET) & INDEX_MASK (SEARCH's
// path walk-back and PushPoint).
H1_ENUM_CONST_BEGIN(MapDirectionConstant)
    MAP_DIRECTION_OPPOSITE_OFFSET = 4,
    MAP_DIRECTION_INDEX_MASK = 7,
    // Odd directions are the diagonals (CalcTerrainCost's diagonal step).
    MAP_DIRECTION_DIAGONAL_BIT = 1
H1_ENUM_CONST_END(MapDirectionConstant)

// The step that walks a map direction back (SEARCH's path walk-back and
// monster back-push).
inline i32 OppositeMapDirection(i32 direction) {
    return (direction + MAP_DIRECTION_OPPOSITE_OFFSET) & MAP_DIRECTION_INDEX_MASK;
}

H1_ENUM_CONST_BEGIN(CursorFrameConstant)
    CURSOR_FRAMES_PER_DIRECTION = 9,
    CURSOR_BOAT_BASE_FRAME_5 = 0x9b,
    CURSOR_BOAT_BASE_FRAME_6 = 0x92,
    CURSOR_BOAT_BASE_FRAME_7 = 0x89
H1_ENUM_CONST_END(CursorFrameConstant)

// advmice.mse frames for mouseManager::SetPointer while the adventure cursor
// set is loaded.
// advManager::ProcessHover picks a role and adds day * DAY_STRIDE for the
// days of travel (0..DAY_LAST); WATER_ACTION + day marks a buoy or whirlpool
// reached by boat. WAIT is shown while another (AI or remote) player moves.
H1_ENUM_BEGIN(AdventurePointerFrame)
    ADVENTURE_POINTER_DEFAULT = 0,
    ADVENTURE_POINTER_WAIT = 1,
    ADVENTURE_POINTER_HERO = 2,
    ADVENTURE_POINTER_TOWN = 3,
    ADVENTURE_POINTER_MOVE = 4,
    ADVENTURE_POINTER_ATTACK = 5,
    ADVENTURE_POINTER_SAIL = 6,
    ADVENTURE_POINTER_DISEMBARK = 7,
    ADVENTURE_POINTER_SELECT_HERO = 8,
    ADVENTURE_POINTER_ACTION = 9,
    ADVENTURE_POINTER_WATER_ACTION = 28
H1_ENUM_END(AdventurePointerFrame)

H1_ENUM_CONST_BEGIN(AdventurePointerConstant)
    ADVENTURE_POINTER_DAY_STRIDE = 6,
    ADVENTURE_POINTER_DAY_LAST = 3
H1_ENUM_CONST_END(AdventurePointerConstant)

#endif

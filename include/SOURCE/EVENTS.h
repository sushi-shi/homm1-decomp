#ifndef HOMM1_SOURCE_EVENTS_H
#define HOMM1_SOURCE_EVENTS_H

#include <Domains.h>

// clang-format off
// mapCell::m_objectMetadata payloads DoEvent/DoAIEvent decode per object
// (Buka EVENTS.h MapObjectEncodingConstant, HoMM1 numbering): campfire
// packed resource/amount, skeleton, artifact pickup modes, daemon cave
// outcomes, graveyard/shipwreck ghost sites, windmill and resource piles,
// and the wandering monster's willing-to-join flag over its count.
H1_ENUM_CONST_BEGIN(MapObjectEncodingConstant)
    MAP_EVENT_DATA_EMPTY = 0,
    MAP_EVENT_DATA_AVAILABLE = 1,
    CAMPFIRE_RESOURCE_MASK = 0xf,
    CAMPFIRE_AMOUNT_SHIFT = 4,
    SKELETON_EMPTY = 1,
    SKELETON_ARTIFACT = 2,
    ARTIFACT_EVENT_MODE_PICKUP = 1,
    ARTIFACT_EVENT_MODE_GUARDED = 2,
    ARTIFACT_EVENT_MODE_GOLD = 3,
    DAEMON_CAVE_EMPTY = 1,
    DAEMON_REWARD_EXPERIENCE = 2,
    DAEMON_REWARD_ARTIFACT = 3,
    DAEMON_REWARD_EXPERIENCE_GOLD = 4,
    DAEMON_REWARD_RANSOM = 5,
    GHOST_SITE_EMPTY = 1,
    GHOST_SITE_SMALL = 2,
    GHOST_SITE_MEDIUM = 3,
    GHOST_SITE_LARGE = 4,
    GHOST_SITE_HUGE = 5,
    WINDMILL_RESOURCE_LAST = 6,
    MAP_EVENT_SPELL_OFFSET = 1,
    WINDMILL_EMPTY = 99,
    RESOURCE_PILE_OBJECT_BASE = 0x3d,
    MONSTER_WILLING_FLAG = 0x80,
    MONSTER_COUNT_MASK = 0x7f
H1_ENUM_CONST_END(MapObjectEncodingConstant)

// Event rewards, costs and guards (Buka MapEventRewardConstant names where
// HoMM1 has the same event).
H1_ENUM_CONST_BEGIN(MapEventRewardConstant)
    MINE_GOLD_INCOME = 1000,
    MINE_ORE_INCOME = 2,
    MINE_RARE_INCOME = 1,
    SAWMILL_WOOD_INCOME = 2,
    ALCHEMIST_MERCURY_INCOME = 1,
    DRAGON_CITY_GOLD_INCOME = 1000,
    DRAGON_CITY_DRAGON_COUNT = 5,
    DRAGON_CITY_CAMPAIGN_DRAGON_COUNT = 20,
    DRAGON_CITY_CAMPAIGN_SCENARIO = 8,
    CHEST_GOLD_MULTIPLIER = 500,
    CHEST_EXPERIENCE_MULTIPLIER = 500,
    CHEST_EXPERIENCE_LEVEL_OFFSET = 1,
    SKELETON_GOLD = 1000,
    CAMPFIRE_GOLD_MULTIPLIER = 100,
    GAZEBO_EXPERIENCE = 1000,
    WATERWHEEL_GOLD_MULTIPLIER = 500,
    RESOURCE_PILE_GOLD_MULTIPLIER = 100,
    WINDMILL_RESOURCE_AMOUNT = 2,
    TEMPLE_MORALE_BONUS = 2,
    ARTIFACT_EVENT_GUARD_ROGUE_COUNT = 50,
    ARTIFACT_EVENT_GOLD_COST = 2000,
    EVENT_RANDOM_ARTIFACT_GOLD = 1000,
    DAEMON_EXPERIENCE = 1000,
    DAEMON_GOLD = 2500,
    GHOST_SMALL_COUNT = 10,
    GHOST_SMALL_GOLD = 1000,
    GHOST_MEDIUM_COUNT = 15,
    GHOST_MEDIUM_GOLD = 2000,
    GHOST_LARGE_COUNT = 25,
    GHOST_LARGE_GOLD = 5000,
    GHOST_HUGE_COUNT = 50,
    GHOST_HUGE_GOLD = 2000
H1_ENUM_CONST_END(MapEventRewardConstant)

// FizzleCenter's sample: the hero-loss fade or the pickup chime (Buka
// EVENT_FIZZLE_HERO_LOSS / EVENT_FIZZLE_ARTIFACT; HoMM1 plays "pickup%02d"
// for every erased pickup).
H1_ENUM_BEGIN(EventFizzleType)
    EVENT_FIZZLE_HERO_LOSS = 0,
    EVENT_FIZZLE_PICKUP = 1
H1_ENUM_END(EventFizzleType)

// GiveTakeArtifactStat's direction (Buka EVENT_ARTIFACT_TAKE).
H1_ENUM_BEGIN(EventArtifactStat)
    EVENT_ARTIFACT_GIVE = 0,
    EVENT_ARTIFACT_TAKE = 1
H1_ENUM_END(EventArtifactStat)

// Screen and distance constants of the event effects (Buka EVENTS.h
// MapEventDisplayConstant / MapEventSpatialConstant / EventEffectConstant,
// HoMM1 values).
H1_ENUM_CONST_BEGIN(MapEventDisplayConstant)
    COAST_FIZZLE_X = 0xc0,
    COAST_FIZZLE_Y = 0xc0,
    COAST_FIZZLE_WIDTH = 0x60,
    COAST_FIZZLE_HEIGHT = 0x60,
    EVENT_FIZZLE_X = 180,
    EVENT_FIZZLE_Y = 172,
    EVENT_FIZZLE_WIDTH = 120,
    EVENT_FIZZLE_HEIGHT = 120,
    EVENT_FIZZLE_STEPS = 65,
    STONE_LITHS_MIN_DISTANCE = 1,
    WHIRLPOOL_MIN_DISTANCE = 3,
    ENVIRONMENT_BORDER = 7,
    EVENT_WHIRLPOOL_TRIGGER_ROLL = 1,
    EVENT_WHIRLPOOL_TRIGGER_MAX = 3,
    EVENT_WHIRLPOOL_ARMY_VALUE_LIMIT = 99999999,
    EVENT_TEXT_BUFFER_SIZE = 500,
    EVENT_TEXT_WINDOW_END = 76,
    // giEventMusicVolume when no music volume is parked.
    EVENT_MUSIC_VOLUME_NONE = -1
H1_ENUM_CONST_END(MapEventDisplayConstant)

// DoCombat's network wait marker and memory thresholds (Buka EVENTS.cpp
// CombatFlowConstant names, HoMM1 values).
H1_ENUM_CONST_BEGIN(CombatFlowConstant)
    COMBAT_NETWORK_POLL_X = 30,
    COMBAT_NETWORK_POLL_Y = 30,
    COMBAT_NETWORK_POLL_WIDTH = 4,
    COMBAT_NETWORK_POLL_HEIGHT = 4,
    COMBAT_LOW_MEMORY_LIMIT = 600,
    COMBAT_HIGH_MEMORY_LIMIT = 1450,
    // DoCombat's randomSeed argument when the caller has none; it then draws
    // one in 1..COMBAT_RANDOM_SEED_MAX.
    COMBAT_RANDOM_SEED_NEW = -1,
    COMBAT_RANDOM_SEED_MAX = 1000
H1_ENUM_CONST_END(CombatFlowConstant)
// clang-format on

// EVENTS data (Buka EVENTS.h owner): the parked music volume DoEvent and DoCombat
// restore (-1 when none) and the event-music flag.
extern int giEventMusicVolume;
extern signed char gbEventMusicPlaying;

#endif // HOMM1_SOURCE_EVENTS_H

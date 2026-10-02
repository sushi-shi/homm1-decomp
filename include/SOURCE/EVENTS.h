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
// clang-format on

// EVENTS data (Buka EVENTS.h owner): the assertion records (file literals and
// line base, as in MOUSEMGR), the parked music volume DoEvent and DoCombat
// restore (-1 when none) and the event-music flag.
extern short gEventsAssertLine;
extern int giEventMusicVolume;
extern signed char gbEventMusicPlaying;

#endif // HOMM1_SOURCE_EVENTS_H

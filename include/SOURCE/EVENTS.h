#ifndef HOMM1_SOURCE_EVENTS_H
#define HOMM1_SOURCE_EVENTS_H

#include <Domains.h>
#include <SOURCE/armyGroup.h>
#include <SOURCE/combatTypes.h>
#include <SOURCE/hero.h>
#include <SOURCE/REMOTE.h>
#include <SOURCE/resourceTypes.h>
#include <SOURCE/town.h>

// mapCell::m_objectMetadata payloads DoEvent/DoAIEvent decode per object:
// campfire packed resource/amount, skeleton, artifact pickup modes, daemon
// cave outcomes, graveyard/shipwreck ghost sites, windmill and resource
// piles, and the wandering monster's willing-to-join flag over its count.
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

// Event rewards, costs and guards.
H1_ENUM_CONST_BEGIN(MapEventRewardConstant)
    MINE_GOLD_INCOME = 1000,
    MINE_ORE_INCOME = 2,
    MINE_RARE_INCOME = 1,
    SAWMILL_WOOD_INCOME = 2,
    ALCHEMIST_MERCURY_INCOME = 1,
    DRAGON_CITY_GOLD_INCOME = 1000,
    DRAGON_CITY_DRAGON_COUNT = 5,
    DRAGON_CITY_CAMPAIGN_DRAGON_COUNT = 20,
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

// FizzleCenter's sample: the hero-loss fade or the pickup chime
// ("pickup%02d" plays for every erased pickup).
H1_ENUM_BEGIN(EventFizzleType)
    EVENT_FIZZLE_HERO_LOSS = 0,
    EVENT_FIZZLE_PICKUP = 1
H1_ENUM_END(EventFizzleType)

// GiveTakeArtifactStat's direction.
H1_ENUM_BEGIN(EventArtifactStat)
    EVENT_ARTIFACT_GIVE = 0,
    EVENT_ARTIFACT_TAKE = 1
H1_ENUM_END(EventArtifactStat)

// Screen and distance constants of the event effects.
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
    EVENT_WHIRLPOOL_TRIGGER_ROLL = 1,
    EVENT_WHIRLPOOL_TRIGGER_MAX = 3,
    EVENT_WHIRLPOOL_ARMY_VALUE_LIMIT = 99999999,
    EVENT_TEXT_BUFFER_SIZE = 500
H1_ENUM_CONST_END(MapEventDisplayConstant)

// DoCombat's network wait marker and memory thresholds.
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

// Event-music flag used by the event/audio flow.
extern i8 gEventMusicPlaying;

// Moved from EVENTS.cpp.
// advManager::EventWindow's eventId: the gEventText row it prints, or
// EVENT_TEXT_CUSTOM for caller text. The five houses use
// RECRUIT/RANKS_FULL/EMPTY of the first house plus three rows per house.
// EventWindow prints the rows FIRST..WINDOW_END (half-open) and reports any
// other id but CUSTOM as "Event ID %d".
H1_ENUM_BEGIN(MapEventTextId)
    EVENT_TEXT_CUSTOM = -1,
    EVENT_TEXT_ALCHEMIST_CAPTURED = 0,
    EVENT_TEXT_FIRST = EVENT_TEXT_ALCHEMIST_CAPTURED,
    EVENT_TEXT_SIGNPOST = 1,
    EVENT_TEXT_BUOY_VISITED = 2,
    EVENT_TEXT_BUOY_REWARD = 3,
    EVENT_TEXT_DAEMON_CAVE_EMPTY = 4,
    EVENT_TEXT_DAEMON_CAVE_EXPERIENCE = 5,
    EVENT_TEXT_DAEMON_CAVE_ARTIFACT = 6,
    EVENT_TEXT_DAEMON_CAVE_GOLD = 7,
    EVENT_TEXT_DAEMON_CAVE_RANSOM = 8,
    EVENT_TEXT_DAEMON_CAVE_DEATH = 9,
    EVENT_TEXT_DAEMON_CAVE_PROMPT = 10,
    EVENT_TEXT_TREASURE_CHEST = 11,
    EVENT_TEXT_FAERIE_RING_VISITED = 12,
    EVENT_TEXT_FAERIE_RING_REWARD = 13,
    EVENT_TEXT_CAMPFIRE = 14,
    EVENT_TEXT_FOUNTAIN_VISITED = 15,
    EVENT_TEXT_FOUNTAIN_REWARD = 16,
    EVENT_TEXT_GAZEBO_VISITED = 17,
    EVENT_TEXT_GAZEBO_REWARD = 18,
    EVENT_TEXT_GENIE_LAMP = 19,
    EVENT_TEXT_GRAVEYARD_PROMPT = 20,
    EVENT_TEXT_GRAVEYARD_EMPTY = 21,
    EVENT_TEXT_GRAVEYARD_REWARD = 22,
    EVENT_TEXT_HOUSE_RECRUIT = 23,
    EVENT_TEXT_HOUSE_RANKS_FULL = 24,
    EVENT_TEXT_HOUSE_EMPTY = 25,
    EVENT_TEXT_DRAGON_CITY_PROMPT = 38,
    EVENT_TEXT_DRAGON_CITY_CONQUERED = 39,
    EVENT_TEXT_LIGHTHOUSE_CAPTURED = 40,
    EVENT_TEXT_WATERWHEEL_EMPTY = 41,
    // Mine of resource r: MINE_CAPTURED_BASE + r (ore 43 .. gold 47).
    EVENT_TEXT_MINE_CAPTURED_BASE = 41,
    EVENT_TEXT_WATERWHEEL_REWARD = 42,
    EVENT_TEXT_FOLLOWERS = 48,
    EVENT_TEXT_MONSTER_REFUSAL = 49,
    EVENT_TEXT_OBELISK_REWARD = 50,
    EVENT_TEXT_OBELISK_VISITED = 51,
    EVENT_TEXT_OASIS_VISITED = 52,
    EVENT_TEXT_OASIS_REWARD = 53,
    EVENT_TEXT_RESOURCE_PICKUP = 54,
    EVENT_TEXT_SAWMILL_CAPTURED = 55,
    EVENT_TEXT_RANKING_SHRINE = 56,
    EVENT_TEXT_SPELL_SHRINE = 57,
    EVENT_TEXT_SHIPWRECK_PROMPT = 58,
    EVENT_TEXT_SHIPWRECK_EMPTY = 59,
    EVENT_TEXT_SHIPWRECK_REWARD = 60,
    EVENT_TEXT_STATUE_REWARD = 61,
    EVENT_TEXT_STATUE_VISITED = 62,
    EVENT_TEXT_DESERT_TENT_EMPTY = 63,
    EVENT_TEXT_DESERT_TENT_RECRUIT = 64,
    EVENT_TEXT_WAGON_EMPTY = 65,
    EVENT_TEXT_WAGON_RECRUIT = 66,
    EVENT_TEXT_WINDMILL_EMPTY = 68,
    EVENT_TEXT_WINDMILL_REWARD = 69,
    EVENT_TEXT_ARTIFACT_GUARDED = 70,
    EVENT_TEXT_LEPRECHAUN_OFFER = 71,
    EVENT_TEXT_LEPRECHAUN_REFUSAL = 72,
    EVENT_TEXT_LEPRECHAUN_NO_GOLD = 73,
    EVENT_TEXT_ARTIFACT_RECOVERED = 74,
    EVENT_TEXT_SKELETON_EMPTY = 75,
    EVENT_TEXT_WINDOW_END = 76,
    EVENT_TEXT_SKELETON_ARTIFACT = 76,
    EVENT_TEXT_COUNT = 77
H1_ENUM_END(MapEventTextId)
extern H1_ENUM_ARRAY(char*, gEventText, MapEventTextId, EVENT_TEXT_COUNT);

// HouseEvent's five recruiting houses (straw hut .. ): three gEventText rows
// each and one creature each.
H1_ENUM_CONST_BEGIN(HouseEventConstant)
    EVENT_TEXT_HOUSE_STRIDE = 3,
    EVENT_HOUSE_COUNT = 5
H1_ENUM_CONST_END(HouseEventConstant)

// The gEventText row a house prints: the first house's RECRUIT, RANKS_FULL
// or EMPTY row offset by three rows per house; and the row announcing a
// captured mine of a resource.
#if H1_STRICT_DOMAINS
inline constexpr MapEventTextId EventTextHouse(int house, MapEventTextId row) {
    return static_cast<MapEventTextId>(house * EVENT_TEXT_HOUSE_STRIDE + static_cast<int>(row));
}
inline constexpr MapEventTextId EventTextMineCaptured(ResourceType resource) {
    return static_cast<MapEventTextId>(
        static_cast<int>(resource) + static_cast<int>(EVENT_TEXT_MINE_CAPTURED_BASE)
    );
}
#define EVENT_TEXT_HOUSE(house, row) EventTextHouse(house, row)
#define EVENT_TEXT_MINE_CAPTURED(resource) EventTextMineCaptured(resource)
#else
#define EVENT_TEXT_HOUSE(house, row) ((house) * EVENT_TEXT_HOUSE_STRIDE + (row))
#define EVENT_TEXT_MINE_CAPTURED(resource) ((resource) + EVENT_TEXT_MINE_CAPTURED_BASE)
#endif

// Remote combat hand-off: SendHeroTownData sends the combat record as
// REMOTE_COMMAND_HERO_TOWN_DATA (answered by REMOTE_COMMAND_HERO_TOWN_CONFIRM),
// then each hero in its own fragment.
H1_ENUM_CONST_BEGIN(CombatRemoteConstant)
    COMBAT_REMOTE_FRAGMENT_COMBAT = 0,
    COMBAT_REMOTE_FRAGMENT_FIRST_HERO = 1,
    COMBAT_REMOTE_FRAGMENT_SECOND_HERO = 2,
    COMBAT_REMOTE_BUFFER_SIZE = 0xff
H1_ENUM_CONST_END(CombatRemoteConstant)

// SendHeroTownData's payload after the remote-message header; hero records
// follow one fragment byte.
#pragma pack(push, 1)
struct combatRemoteData {
    i8 fragment;
    i8 x;
    i8 y;
    b8 hasFirstHero;
    b8 hasTown;
    b8 hasSecondHero;
    i8 setupCombatX;
    i8 setupCombatY;
    i32 randomSeed;
    i8 combatResult;
    i8 retreatWin;
    i8 combatSurrender;
    i8 firstOwner;
    i32 firstGold;
    i8 secondOwner;
    i32 secondGold;
    armyGroup firstArmy;
    armyGroup secondArmy;
    town combatTown;
};

struct combatRemoteHeroFragment {
    i8 fragment;
    char data[sizeof(hero)];
};

struct combatRemoteMessage {
    i8 sender;
    i32 id;
    H1_ENUM_STORAGE(RemoteMessageType, i8) type;
    i8 command;
    i16 payloadSize;
    combatRemoteData combat;
};

struct heroRemoteMessage {
    i8 sender;
    i32 id;
    H1_ENUM_STORAGE(RemoteMessageType, i8) type;
    i8 command;
    i16 payloadSize;
    combatRemoteHeroFragment heroFragment;
};
#pragma pack(pop)

// ReceiveHeroTownData reads a received RemoteMessage record's payload as the
// combat record or a hero fragment. combatRemoteData holds armyGroup and town
// objects, whose constructors keep it out of the RemotePayload union.
#define EVENTS_REMOTE_MESSAGE(record)                                                              \
    (reinterpret_cast<combatRemoteMessage*>(record)) // Overlay: the combat payload.
#define EVENTS_REMOTE_HERO(record)                                                                 \
    (reinterpret_cast<heroRemoteMessage*>(record)) // Overlay: a hero fragment payload.

#endif // HOMM1_SOURCE_EVENTS_H

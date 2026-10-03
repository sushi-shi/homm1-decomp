#ifndef HOMM1_SOURCE_MAPOBJECTTYPES_H
#define HOMM1_SOURCE_MAPOBJECTTYPES_H

#include <Domains.h>

// mapCell::m_triggerType: the low seven bits select the object, the high bit
// marks the cell whose entry runs the object's event (DoEvent dispatches on
// the masked type only when the bit is set; RandomizeEvents clears it from
// passive object types).
H1_ENUM_CONST_BEGIN(MapTriggerEncoding)
    MAP_TRIGGER_TYPE_MASK = 0x7f,
    MAP_TRIGGER_EVENT = 0x80
H1_ENUM_CONST_END(MapTriggerEncoding)

// Adventure-map object types. Names follow retail gObjectNames (0x00493280),
// which advManager::QuickInfo prints for the masked type, and the
// advManager::DoEvent arm each value runs. Equal retail strings are split by
// their arm: houses 13..17 recruit HouseEvent's goblins, peasants, archers,
// dwarves and peasants; shrine 33 shows the player rankings and shrine 34
// teaches gSpellNames[metadata - 1]. Retail names 26 "Army Camp" but every
// reader treats it as a wandering monster stack, and names 31 "Sandpit" but
// every reader uses it as the coast cell where an embarked hero lands.
// Mountain and tree variants 52..55 and 56..60 fold to their first member in
// GetObjectFamily (gObjectNames repeats "Mountains"/"Trees"); the variants are
// numbered after it, since no table names them individually.
// 61 and 62 are the runtime hero and boat; the map file uses the same codes
// for placeholders (MapFileObjectType).
H1_ENUM_BEGIN(MapObjectType)
    MAP_OBJECT_NONE = 0,
    MAP_OBJECT_ALCHEMIST_LAB = 1,
    MAP_OBJECT_SIGNPOST = 2,
    MAP_OBJECT_BUOY = 3,
    MAP_OBJECT_SKELETON = 4,
    MAP_OBJECT_DAEMON_CAVE = 5,
    MAP_OBJECT_TREASURE_CHEST = 6,
    MAP_OBJECT_FAERIE_RING = 7,
    MAP_OBJECT_CAMPFIRE = 8,
    MAP_OBJECT_FOUNTAIN = 9,
    MAP_OBJECT_GAZEBO = 10,
    MAP_OBJECT_ANCIENT_LAMP = 11,
    MAP_OBJECT_GRAVEYARD = 12,
    MAP_OBJECT_STRAW_HUT = 13,
    MAP_OBJECT_HOUSE_FIRST = MAP_OBJECT_STRAW_HUT,
    MAP_OBJECT_HOUSE = 14,
    MAP_OBJECT_CABIN = 15,
    MAP_OBJECT_DWARF_LOG_CABIN = 16,
    MAP_OBJECT_PEASANT_LOG_CABIN = 17,
    MAP_OBJECT_INN_1 = 18,
    MAP_OBJECT_INN_2 = 19,
    MAP_OBJECT_INN_3 = 20,
    MAP_OBJECT_INN_4 = 21,
    MAP_OBJECT_DRAGON_CITY = 22,
    MAP_OBJECT_LIGHTHOUSE = 23,
    MAP_OBJECT_WATERWHEEL = 24,
    MAP_OBJECT_MINE = 25,
    MAP_OBJECT_MONSTER = 26,
    MAP_OBJECT_OBELISK = 27,
    MAP_OBJECT_OASIS = 28,
    MAP_OBJECT_RESOURCE = 29,
    MAP_OBJECT_ROSEBUSH = 30,
    MAP_OBJECT_COAST = 31,
    MAP_OBJECT_SAWMILL = 32,
    MAP_OBJECT_RANKING_SHRINE = 33,
    MAP_OBJECT_SPELL_SHRINE = 34,
    MAP_OBJECT_SHIPWRECK = 35,
    MAP_OBJECT_STATUE = 36,
    MAP_OBJECT_TREE_STUMP = 37,
    MAP_OBJECT_SWAN_POND = 38,
    MAP_OBJECT_DESERT_TENT = 39,
    MAP_OBJECT_TOWN = 40,
    MAP_OBJECT_STONE_LITHS = 41,
    MAP_OBJECT_WAGON_CAMP = 42,
    MAP_OBJECT_WELL = 43,
    MAP_OBJECT_WHIRLPOOL = 44,
    MAP_OBJECT_WINDMILL = 45,
    MAP_OBJECT_OAK_TREE = 46,
    MAP_OBJECT_MEGALITH = 47,
    MAP_OBJECT_ARTIFACT = 48,
    // The event objects end here; NOTHING_HERE and the mountain/tree
    // obstacles after it run no event (RandomizeTown keeps an event object
    // under a placed town as the cell's secondary trigger).
    MAP_OBJECT_EVENT_LAST = MAP_OBJECT_ARTIFACT,
    MAP_OBJECT_NOTHING_HERE = 49,
    // Unnamed in gObjectNames: a shadow cell placed without an event bit;
    // RandomizeEvents marks it MAP_CELL_OBJECT_SHADOW_ONLY.
    MAP_OBJECT_SHADOW = 50,
    MAP_OBJECT_MOUNTAINS = 52,
    MAP_OBJECT_MOUNTAINS_2 = 53,
    MAP_OBJECT_MOUNTAINS_3 = 54,
    MAP_OBJECT_MOUNTAINS_4 = 55,
    MAP_OBJECT_MOUNTAINS_LAST = MAP_OBJECT_MOUNTAINS_4,
    MAP_OBJECT_TREES = 56,
    MAP_OBJECT_TREES_2 = 57,
    MAP_OBJECT_TREES_3 = 58,
    MAP_OBJECT_TREES_4 = 59,
    MAP_OBJECT_TREES_5 = 60,
    MAP_OBJECT_TREES_LAST = MAP_OBJECT_TREES_5,
    // NOTHING_HERE..TREES_LAST: the non-event objects philAI's
    // ValueOfEventAtPosition values at zero.
    MAP_OBJECT_NON_EVENT_FIRST = MAP_OBJECT_NOTHING_HERE,
    MAP_OBJECT_HERO = 61,
    MAP_OBJECT_SHIP = 62,
    // Past gObjectNames: the dug-up ultimate artifact (EventSound plays
    // MUSIC_TRACK_ULTIMATE_ARTIFACT for it; game keeps its event bit).
    MAP_OBJECT_ULTIMATE_ARTIFACT = 63
H1_ENUM_END(MapObjectType)

// Map-file object codes that game::ProcessRandomObjects and the hero setup
// replace before play: random towns and castles (RandomizeTown 0/1), monster
// fight-value bands 80..2000, 0..400, 80..1000, 500..2500 and 2000..100000,
// resources (object index 61..67), artifacts, mines, and placed heroes whose
// mapHeroExtra record the metadata selects.
H1_ENUM_BEGIN(MapFileObjectType)
    MAP_FILE_OBJECT_RANDOM_ARTIFACT = 61,
    MAP_FILE_OBJECT_RANDOM_RESOURCE = 62,
    MAP_FILE_OBJECT_RANDOM_MONSTER = 63,
    MAP_FILE_OBJECT_RANDOM_TOWN = 64,
    MAP_FILE_OBJECT_RANDOM_CASTLE = 65,
    MAP_FILE_OBJECT_RANDOM_MINE = 66,
    MAP_FILE_OBJECT_RANDOM_MONSTER_WEAK = 67,
    MAP_FILE_OBJECT_RANDOM_MONSTER_MEDIUM = 68,
    MAP_FILE_OBJECT_RANDOM_MONSTER_STRONG = 69,
    MAP_FILE_OBJECT_RANDOM_MONSTER_VERY_STRONG = 70,
    MAP_FILE_OBJECT_HERO = 71
H1_ENUM_END(MapFileObjectType)

#endif // HOMM1_SOURCE_MAPOBJECTTYPES_H

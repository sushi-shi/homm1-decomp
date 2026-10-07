#ifndef HOMM1_SOURCE_GAMETYPES_H
#define HOMM1_SOURCE_GAMETYPES_H

enum GameStorageConstant {
    GAME_PLAYER_COUNT = 4,
    GAME_MIN_PLAYER_COUNT = 2,
    GAME_TOWN_COUNT = 36,
    GAME_HERO_COUNT = 36,
    GAME_MINE_COUNT = 36,
    GAME_BOAT_COUNT = 32,
    // One bit per town; the original game kept four bytes, so towns 32-35
    // used the bits of the first hero's id.
    GAME_TOWN_FLAG_BYTES = (GAME_TOWN_COUNT + 7) / 8,
    GAME_TOWN_FLAG_BYTES_ORIGINAL = 4
};

enum GamePlayerConstant {
    GAME_PLAYER_NONE = -1,
    GAME_TOWN_NONE = -1,
    GAME_MINE_NONE = -1,
    GAME_TABLE_FREE = -1,
    GAME_RANDOM_SCAN_TRIES = 10000,
    GAME_PLAYER_HIGH_BIT_SHIFT = 4,
    GAME_ARTIFACT_ON_MAP = 36,
    // CellRandomArtifactId draws a site's seed from the first heroes.
    GAME_ARTIFACT_SEED_HERO_COUNT = 31
};

enum GameCalendarConstant {
    CALENDAR_DAYS_PER_WEEK = 7,
    CALENDAR_DAYS_PER_MONTH = 28,
    CALENDAR_WEEKS_PER_MONTH = 4
};

#endif

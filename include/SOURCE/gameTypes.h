#ifndef HOMM1_SOURCE_GAMETYPES_H
#define HOMM1_SOURCE_GAMETYPES_H

#include <Domains.h>

// game object table extents (players, towns, heroes, mines, boats); the
// per-player records size their town lists and locators with them too.
H1_ENUM_CONST_BEGIN(GameStorageConstant)
    GAME_PLAYER_COUNT = 4,
    // NewGameHandler refuses a game with fewer seats in play.
    GAME_MIN_PLAYER_COUNT = 2,
    GAME_TOWN_COUNT = 36,
    GAME_HERO_COUNT = 36,
    GAME_MINE_COUNT = 36,
    GAME_BOAT_COUNT = 32
H1_ENUM_CONST_END(GameStorageConstant)

// The -1 "none" of the game table indices. A player index (game::m_players,
// giCurPlayer, town/hero/castle/mine owners, combatManager::m_playerId) is
// NONE for the neutral owner of an unclaimed town or mine and the monster
// side of a combat (Buka TOWN_OWNER_NONE / HERO_OWNER_NONE); a hero or town
// id (playerData::m_currentHero/m_heroIds, m_currentTown/m_townIds,
// game::m_availableHeroes) is NONE for an empty slot (Buka TOWN_ID_NONE).
// game::Scan/RandomScan look for a FREE (-1) entry of a signed-char table
// (m_boatSlots, m_availableHeroes, m_townOwners); RandomScan gives up after
// RANDOM_SCAN_TRIES rolls. A player's "high" bit is its bit in the upper
// nibble of a per-player byte (gCurPlayerTopBit = 1 << (p + SHIFT)).
H1_ENUM_CONST_BEGIN(GamePlayerConstant)
    GAME_PLAYER_NONE = -1,
    GAME_HERO_NONE = -1,
    GAME_TOWN_NONE = -1,
    GAME_MINE_NONE = -1,
    GAME_TABLE_FREE = -1,
    GAME_RANDOM_SCAN_TRIES = 10000,
    GAME_PLAYER_HIGH_BIT_SHIFT = 4,
    // game::m_randomArtifacts holds the hero id carrying each random
    // artifact (EVENTS pickup/trade), FREE when unused, and this past-the-
    // hero-table id once ProcessRandomObjects has placed it on the map.
    GAME_ARTIFACT_ON_MAP = 36
H1_ENUM_CONST_END(GamePlayerConstant)

// The calendar (Buka GameCalendarConstant): four seven-day weeks a month.
H1_ENUM_CONST_BEGIN(GameCalendarConstant)
    CALENDAR_DAYS_PER_WEEK = 7,
    CALENDAR_DAYS_PER_MONTH = 28,
    CALENDAR_WEEKS_PER_MONTH = 4
H1_ENUM_CONST_END(GameCalendarConstant)

#endif

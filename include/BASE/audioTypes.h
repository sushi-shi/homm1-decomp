#ifndef HOMM1_BASE_AUDIOTYPES_H
#define HOMM1_BASE_AUDIOTYPES_H

#include <Domains.h>

H1_ENUM_BEGIN(SoundMusicSource)
    SOUND_MUSIC_SOURCE_DIGITAL = 0,
    SOUND_MUSIC_SOURCE_DIGITAL_STEREO = 1,
    SOUND_MUSIC_SOURCE_CD = 2
H1_ENUM_END(SoundMusicSource)

// Logical music tracks for PlayMusic (the Ogg backend maps them to file numbers). 0..6 are the TerrainType themes and the town themes
// start at TOWN_THEME_MUSIC_BASE; the rest are named by the call sites that
// play them: advManager::EventSound's object cues, the battle list in
// combatManager::Open, DoVictory's win/lose cues, and the menu, AI-turn,
// level-up and congratulations screens.
H1_ENUM_BEGIN(MusicTrack)
    MUSIC_TRACK_NONE = -1,
    MUSIC_TRACK_DAEMON_CAVE = 7,
    MUSIC_TRACK_FAERIE_RING = 8,
    MUSIC_TRACK_GAZEBO = 9,
    MUSIC_TRACK_ANCIENT_LAMP = 0xa,
    MUSIC_TRACK_GRAVEYARD = 0xb,
    MUSIC_TRACK_DRAGON_CITY = 0xc,
    MUSIC_TRACK_PUZZLE = 0xd,
    MUSIC_TRACK_STATUE = 0xe,
    // The local human's turn starting in a network game (game::NewWeek,
    // advManager/game turn hand-over with gForceSwitchMusic).
    MUSIC_TRACK_NETWORK_TURN = 0xf,
    // The same cue plays at the desert tent (EVENTS).
    MUSIC_TRACK_DESERT_TENT = MUSIC_TRACK_NETWORK_TURN,
    MUSIC_TRACK_TELEPORT = 0x10,
    MUSIC_TRACK_WAGON_CAMP = 0x11,
    MUSIC_TRACK_BUOY_OASIS = 0x14,
    MUSIC_TRACK_OBELISK = 0x15,
    MUSIC_TRACK_HOUSE = 0x16,
    MUSIC_TRACK_MINE_CAPTURED = 0x17,
    MUSIC_TRACK_FOUNTAIN = 0x18,
    MUSIC_TRACK_WHIRLPOOL = 0x19,
    MUSIC_TRACK_LIGHTHOUSE = 0x1a,
    MUSIC_TRACK_SPELL_SHRINE = 0x1b,
    MUSIC_TRACK_TREASURE = 0x1c,
    MUSIC_TRACK_BATTLE_1 = 0x28,
    MUSIC_TRACK_BATTLE_2 = 0x29,
    MUSIC_TRACK_BATTLE_3 = 0x2a,
    MUSIC_TRACK_BATTLE_LOST = 0x2b,
    MUSIC_TRACK_BATTLE_WON = 0x2c,
    MUSIC_TRACK_ULTIMATE_ARTIFACT = 0x2e,
    MUSIC_TRACK_MAIN_MENU = 0x30,
    MUSIC_TRACK_AI_TURN = 0x31,
    // game::NewDay's new-week and new-month announcements.
    MUSIC_TRACK_NEW_WEEK = 0x32,
    MUSIC_TRACK_NEW_MONTH = 0x33,
    MUSIC_TRACK_LEVEL_UP = 0x34,
    MUSIC_TRACK_BATTLE_4 = 0x35,
    MUSIC_TRACK_CONGRATULATIONS = 0x36
H1_ENUM_END(MusicTrack)

#endif

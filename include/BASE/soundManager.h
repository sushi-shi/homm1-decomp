#ifndef HOMM1_BASE_SOUNDMANAGER_H
#define HOMM1_BASE_SOUNDMANAGER_H
// Reconstructed class (BASE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 37 methods, 3 own-virtual, 0 static data.

#include <BASE/baseManager.h>
#include <Domains.h>
#include <H1/Macros.h>

#include <stdio.h>

H1_ENUM_CONST_BEGIN(SoundManagerConstant)
    MUSIC_TRACK_COUNT = 60,
    MUSIC_POSITION_TRACK_END = 7,
    MUSIC_POSITION_TRACK_1 = 47,
    MUSIC_POSITION_TRACK_2 = 48,
    MUSIC_POSITION_TRACK_3 = 49,
    MUSIC_FADE_HOLD_LAST = 10,
    MUSIC_FADE_TOTAL_STEPS = 11,
    MUSIC_FADE_RISE_STEPS = 6,
    MUSIC_FADE_STEP_TICKS = 60,
    SAMPLE_VOLUME_MAX = 64,
    MIDI_VOLUME_MAX = 127,
    CD_VOLUME_SCALE_DIVISOR = 640,
    SOUND_OPERATION_VOLUME = 1,
    SOUND_OPERATION_START = 5,
    SOUND_OPERATION_EFFECT_VOLUME = 100,
    SOUND_OPERATION_MUSIC_VOLUME = 101,
    MUSIC_STREAM_BUFFER_SIZE = 0x4000,
    MUSIC_STREAM_RATE = 22050,
    SAMPLE_STATUS_PLAYING = 4
H1_ENUM_CONST_END(SoundManagerConstant)

// soundManager's startup sample-handle count and default sample format; the
// manager's priority is BASE_MANAGER_PRIORITY_UNASSIGNED.
H1_ENUM_CONST_BEGIN(SoundStartupConstant)
    SOUND_SAMPLE_HANDLE_COUNT = 15,
    SOUND_DEFAULT_SAMPLE_BITS = 8,
    SOUND_DEFAULT_SAMPLE_CHANNELS = 1
H1_ENUM_CONST_END(SoundStartupConstant)

// gConfig.musicSource ("Sound Quality"; musicQualityText "8 Bit Mono",
// "8 Bit Stereo", "CD Stereo"): PlayMusic streams heroes%02d.82m for mono and
// .82s for stereo; CD plays the disc (m_cdReady) and otherwise streams .62s.
H1_ENUM_BEGIN(SoundMusicSource)
    SOUND_MUSIC_SOURCE_DIGITAL = 0,
    SOUND_MUSIC_SOURCE_DIGITAL_STEREO = 1,
    SOUND_MUSIC_SOURCE_CD = 2
H1_ENUM_END(SoundMusicSource)

// Logical music tracks for SwitchAmbientMusic/PlayAmbientMusic (CDPlay maps
// them to disc tracks). 0..6 are the TerrainType themes and the town themes
// start at TOWN_THEME_MUSIC_BASE; the rest are named by the call sites that
// play them: advManager::EventSound's object cues, the battle list in
// combatManager::Open, DoVictory's win/lose cues, and the menu, AI-turn,
// level-up and congratulations screens. MUSIC_POSITION_TRACK_* marks the
// resumable ones (tavern, main menu, AI turn).
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
    // advManager/game turn hand-over with giForceSwitchMusic).
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

// forward declarations:
class sample;
struct _SAMPLE;
struct tag_message;

#pragma pack(push, 1)
class soundManager : public baseManager {
public:
    struct _DIG_DRIVER* m_digitalDriver;
    struct _SAMPLE* m_activeSample;
    int m_samplesReady;
    struct _SAMPLE* m_musicSample;
    char m_musicStreamOpen;
    char m_musicStreamRestart;
    void* m_musicBuffers[2];
    FILE* m_midiFile;
    struct _SAMPLE* m_sampleHandles[SOUND_SAMPLE_HANDLE_COUNT];
    char _pad_0x08a[4];
    int m_numSampleHandles;
    char _pad_0x092[0x40];
    char m_channelVolumes[0x14];
    struct _SAMPLE* m_channelSamples[14];
    char _pad_0x11e[8];
    void* m_channelSampleData[14];
    char _pad_0x15e[8];
    unsigned long m_channelSampleSizes[14];
    char _pad_0x19e[0x3c8];
    int m_field_0x566;
    char _pad_0x56a[4];
    int m_field_0x56e;
    char m_currentTrack;
    char m_pollRequested;
    char m_pollDue;
    char m_pollToggle;
    char _pad_0x576[0x14];
    long m_savedTrackPositions[60];
    int m_fading;
    int m_musicReady;
    int m_fadeSteps;
    int m_fadeTargetTrack;
    int m_cdTrack;
    int m_cdPlayFrame;
    short m_auxDevice;
    int m_cdReady;
    int m_cdStarted;
    // --- constructors ---
    soundManager(void);
    // --- virtual methods (vtable order) ---
    virtual short Open(short) OVERRIDE;
    virtual void Close(void) OVERRIDE;
    virtual short Main(struct tag_message&) OVERRIDE;
    // --- methods ---
    void ValidatePreviousPosition(int);
    void CDStop(void);
    int CDIsPlaying(void);
    unsigned long CDStartup(void);
    void CDShutdown(void);
    void CDSetVolume(int, int);
    void CDPlay(int, int, int, int);
    void CDPoll(void);
    int ConvertVolume(int, int);
    void AllocateSampleHandles(void);
    struct _SAMPLE* StartSample(char*, char**, short int, short int, int, int, long int);
    void StopAllSamples(void);
    void StopSample(struct _SAMPLE*);
    void ModifySample(struct _SAMPLE*, short int, long int);
    long int DigitalReport(struct _SAMPLE*, short int);
    void AdjustSoundVolumes(void);
    void AdjustMusicVolumes(void);
    void ForcePollSound(void);
    void SetMusicQuality(int);
    void PlayAmbientMusic(int, long int, int);
    void PollSound(void);
    void SwitchAmbientMusic(int);
    struct _SAMPLE* MemorySample(class sample*);
    void GetNumberCDDrives(void);
    void ServiceSound(void);
    int MusicPlaying(void);
    void MIDIStartup(void);
    void MIDIShutdown(void);
    void MIDIPlay(int midiTrack);
    void MIDIStop(void);
    int MIDIIsPlaying(void);
    void MIDISetVolume(void);
    void MIDIPoll(void);
};
#pragma pack(pop)
#endif // HOMM1_BASE_SOUNDMANAGER_H

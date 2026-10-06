#ifndef HOMM1_BASE_SOUNDMANAGER_H
#define HOMM1_BASE_SOUNDMANAGER_H

#include <BASE/baseManager.h>

#include <stdio.h>

enum SoundManagerConstant {
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
};

enum SoundStartupConstant {
    SOUND_SAMPLE_HANDLE_COUNT = 15,
    SOUND_DEFAULT_SAMPLE_BITS = 8,
    SOUND_DEFAULT_SAMPLE_CHANNELS = 1
};

enum SoundMusicSource {
    SOUND_MUSIC_SOURCE_DIGITAL = 0,
    SOUND_MUSIC_SOURCE_DIGITAL_STEREO = 1,
    SOUND_MUSIC_SOURCE_CD = 2
};

enum MusicTrack {
    MUSIC_TRACK_NONE = -1,
    MUSIC_TRACK_DAEMON_CAVE = 7,
    MUSIC_TRACK_FAERIE_RING = 8,
    MUSIC_TRACK_GAZEBO = 9,
    MUSIC_TRACK_ANCIENT_LAMP = 0xa,
    MUSIC_TRACK_GRAVEYARD = 0xb,
    MUSIC_TRACK_DRAGON_CITY = 0xc,
    MUSIC_TRACK_PUZZLE = 0xd,
    MUSIC_TRACK_STATUE = 0xe,
    MUSIC_TRACK_NETWORK_TURN = 0xf,
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
    MUSIC_TRACK_NEW_WEEK = 0x32,
    MUSIC_TRACK_NEW_MONTH = 0x33,
    MUSIC_TRACK_LEVEL_UP = 0x34,
    MUSIC_TRACK_BATTLE_4 = 0x35,
    MUSIC_TRACK_CONGRATULATIONS = 0x36
};

class sample;
struct _SAMPLE;
struct tag_message;

#pragma pack(push, 1)
class soundManager : public baseManager {
public:
    struct _DIG_DRIVER* m_digitalDriver;
    struct _SAMPLE* m_activeSample;
    i32 m_samplesReady;
    struct _SAMPLE* m_musicSample;
    i8 m_musicStreamOpen;
    i8 m_musicStreamRestart;
    void* m_musicBuffers[2];
    FILE* m_midiFile;
    struct _SAMPLE* m_sampleHandles[SOUND_SAMPLE_HANDLE_COUNT];
    char _pad_0x08a[4];
    i32 m_numSampleHandles;
    char _pad_0x092[0x40];
    char m_channelVolumes[0x14];
    struct _SAMPLE* m_channelSamples[14];
    char _pad_0x11e[8];
    void* m_channelSampleData[14];
    char _pad_0x15e[8];
    u32 m_channelSampleSizes[14];
    char _pad_0x19e[0x3c8];
    i32 m_field_0x566;
    char _pad_0x56a[4];
    i32 m_field_0x56e;
    i8 m_currentTrack;
    i8 m_pollRequested;
    i8 m_pollDue;
    i8 m_pollToggle;
    char _pad_0x576[0x14];
    i32 m_savedTrackPositions[60];
    i32 m_fading;
    i32 m_musicReady;
    i32 m_fadeSteps;
    i32 m_fadeTargetTrack;
    i32 m_cdTrack;
    i32 m_cdPlayFrame;
    i16 m_auxDevice;
    i32 m_cdReady;
    i32 m_cdStarted;
    soundManager(void);
    virtual i16 Open(i16) ;
    virtual void Close(void) ;
    virtual i16 Main(struct tag_message&) ;
    void ValidatePreviousPosition(i32 track);
    void CDStop(void);
    i32 CDIsPlaying(void);
    u32 CDStartup(void);
    void CDShutdown(void);
    void CDSetVolume(i32 volume, i32 fadeScale);
    void CDPlay(i32 track, i32 resume, i32 volume, i32 restart);
    void CDPoll(void);
    i32 ConvertVolume(i32 volume, i32 soundType);
    void AllocateSampleHandles(void);
    struct _SAMPLE*
    StartSample(char* name, char**, i16, i16 loop, i32 volume, i32 channelType, i32 resume);
    void StopAllSamples(void);
    void StopSample(struct _SAMPLE* sample);
    void ModifySample(struct _SAMPLE* sampleHandle, i16 operation, i32 value);
    i32 DigitalReport(struct _SAMPLE* sample, i16 reportType);
    void AdjustSoundVolumes(void);
    void AdjustMusicVolumes(void);
    void ForcePollSound(void);
    void SetMusicQuality(i32 musicSource);
    void PlayAmbientMusic(i32 track, i32 resume, i32 volume);
    void PollSound(void);
    void SwitchAmbientMusic(i32 track);
    struct _SAMPLE* MemorySample(class sample* sampleResource);
    void GetNumberCDDrives(void);
    void ServiceSound(void);
    i32 MusicPlaying(void);
    void MIDIStartup(void);
    void MIDIShutdown(void);
    void MIDIPlay(i32 midiTrack);
    void MIDIStop(void);
    i32 MIDIIsPlaying(void);
    void MIDISetVolume(void);
    void MIDIPoll(void);
};
#pragma pack(pop)
#endif

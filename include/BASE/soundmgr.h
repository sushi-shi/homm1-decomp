#ifndef HOMM1_BASE_SOUNDMGR_H
#define HOMM1_BASE_SOUNDMGR_H

#include <Domains.h>

struct _DIG_DRIVER;
struct pcmwaveformat_tag;
struct tagAUXCAPSA;
extern pcmwaveformat_tag gWaveFormat;
extern tagAUXCAPSA gAuxCaps;
_DIG_DRIVER* WAVE_init_driver(u32 sampleRate, u16 bitsPerSample, u16 channels, u16 showErrors);

H1_ENUM_CONST_BEGIN(CDPlaybackConstant)
    CD_POSITION_BUFFER_SIZE = 20,
    CD_POSITION_CAPACITY = 15,
    CD_MCI_RESULT_LAST = 255,
    CD_VOLUME_LEVEL_COUNT = 12,
    CD_VOLUME_LEVEL_SHIFT = 12,
    CD_STEREO_CHANNEL_SHIFT = 16,
    CD_FADE_DELAY_TICKS = 480,
    CD_NOTIFY_FIRST = 40,
    CD_NOTIFY_LAST = 42,
    CD_NOTIFY_EXTRA_FIRST = 53,
    CD_NOTIFY_EXTRA_LAST = 54,
    CD_NOTIFY_SCENARIO_FIRST = 29,
    CD_NOTIFY_SCENARIO_LAST = 32,
    MUSIC_FILENAME_CAPACITY = 40,
    MUSIC_STOP_WAIT_COUNT = 10,
    SAMPLE_STOP_ALL_WAIT_COUNT = 20,
    SAMPLE_REACQUIRE_WAIT_MILLISECONDS = 5,
    AMBIENT_FADE_DELAY_TICKS = 900,
    SAMPLE_STATUS_DONE = 2,
    SAMPLE_VOLUME_TABLE_BYTES = 0x40,
    SOUND_STATE_RESET_SPAN = 0xae,
    MUSIC_STOP_WAIT_MILLISECONDS = 5,
    // soundManager::m_auxDevice before CDStartup finds a CD-audio aux device.
    CD_AUX_DEVICE_NONE = -1
H1_ENUM_CONST_END(CDPlaybackConstant)

H1_ENUM_CONST_BEGIN(SampleStreamConstant)
    SAMPLE_PATH_CAPACITY = 352,
    SAMPLE_SUFFIX_COUNT = 3,
    SAMPLE_RATE_LOW = 11025,
    SAMPLE_RATE_NORMAL = 22050,
    SAMPLE_RATE_HIGH = 44100,
    SAMPLE_FORMAT_16_BIT = 1,
    SAMPLE_FORMAT_STEREO = 2,
    // gConfig.musicVolume/soundVolume level that silences the channel.
    SOUND_VOLUME_OFF = 0,
    SOUND_VOLUME_FIRST = 1,
    SOUND_VOLUME_LAST = 10,
    SOUND_VOLUME_EFFECT = 100,
    SOUND_VOLUME_MUSIC = 101,
    PCM_BITS_PER_BYTE_SHIFT = 3,
    // Volume argument meaning "use the configured music volume".
    SOUND_VOLUME_FROM_CONFIG = -1
H1_ENUM_CONST_END(SampleStreamConstant)

// gConfig.musicVolume/soundVolume levels as the system menu labels them:
// level 1 is full volume and level 10 the quietest (gVolumeLevels).
H1_ENUM_CONST_BEGIN(ConfigVolumeLevel)
    SOUND_VOLUME_100 = 1,
    SOUND_VOLUME_90 = 2,
    SOUND_VOLUME_80 = 3,
    SOUND_VOLUME_70 = 4,
    SOUND_VOLUME_60 = 5,
    SOUND_VOLUME_50 = 6,
    SOUND_VOLUME_40 = 7,
    SOUND_VOLUME_30 = 8,
    SOUND_VOLUME_20 = 9,
    SOUND_VOLUME_10 = 10
H1_ENUM_CONST_END(ConfigVolumeLevel)

H1_ENUM_BEGIN(SampleReportQuery)
    SAMPLE_REPORT_VOLUME = 1,
    SAMPLE_REPORT_PLAYING = 4
H1_ENUM_END(SampleReportQuery)

extern i32 CDPlaying;
extern i32 CDPlayOnce;
extern i8 CDTrackMap[];
extern char CDPreviousPosition[][CD_POSITION_CAPACITY];
extern char CommandString[];
extern char lpszReturnString[];
extern u32 nMCIError;
extern i16 gSampleVolumes[];
struct SampleChannelStruct {
    i32 startChannel;
    i32 endChannel;
    i32 currentChannel;
};
extern SampleChannelStruct SCS[];

void SetReady2Poll(void);
void HandleMCIError(i32 errorCode, char* command);

#endif

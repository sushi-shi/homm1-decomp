#ifndef HOMM1_BASE_SOUNDMGR_H
#define HOMM1_BASE_SOUNDMGR_H

#include <Domains.h>

struct _DIG_DRIVER;
struct pcmwaveformat_tag;
struct tagAUXCAPSA;
extern pcmwaveformat_tag gWaveFormat;
extern tagAUXCAPSA gAuxCaps;
_DIG_DRIVER* WAVE_init_driver(unsigned long, unsigned short, unsigned short, unsigned short);

H1_ENUM_BEGIN(CDPlaybackConstant)
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
    SAMPLE_STOP_ALL_WAIT_COUNT = 5,
    SAMPLE_VOLUME_TABLE_BYTES = 0x40,
    SOUND_STATE_RESET_SPAN = 0xae,
    MUSIC_STOP_WAIT_MILLISECONDS = 5
H1_ENUM_END(CDPlaybackConstant)

H1_ENUM_BEGIN(SampleStreamConstant)
    SAMPLE_PATH_CAPACITY = 352,
    SAMPLE_SUFFIX_COUNT = 3,
    SAMPLE_RATE_LOW = 11025,
    SAMPLE_RATE_NORMAL = 22050,
    SAMPLE_RATE_HIGH = 44100,
    SAMPLE_FORMAT_16_BIT = 1,
    SAMPLE_FORMAT_STEREO = 2,
    SOUND_VOLUME_FIRST = 1,
    SOUND_VOLUME_LAST = 10,
    SOUND_VOLUME_EFFECT = 100,
    SOUND_VOLUME_MUSIC = 101,
    PCM_BITS_PER_BYTE_SHIFT = 3
H1_ENUM_END(SampleStreamConstant)

H1_ENUM_BEGIN(SampleReportQuery)
    SAMPLE_REPORT_VOLUME = 1,
    SAMPLE_REPORT_PLAYING = 4
H1_ENUM_END(SampleReportQuery)

extern int CDPlaying;
extern int CDPlayOnce;
extern signed char CDTrackMap[];
extern char CDPreviousPosition[][CD_POSITION_CAPACITY];
extern char CommandString[];
extern char lpszReturnString[];
extern unsigned long nMCIError;
extern short gCDPositionAssertLine;
extern char gCDPositionAssertFile[];
extern short gAmbientMusicAssertLine;
extern char gAmbientMusicAssertFile[];
extern short gStartSampleAssertLine;
extern char gStartSampleAssertFile[];
extern short gStopAllSamplesAssertLine;
extern char gStopAllSamplesAssertFile[];
extern short gModifySampleAssertLine;
extern char gModifySampleAssertFile[];
extern short gAdjustMusicAssertLine;
extern char gAdjustMusicAssertFile[];
extern short gSampleVolumes[];
extern char gcSoundPath[];
extern char gcDataPath[];
char* FindToken(char*, char);
void SetReady2Poll(void);
void HandleMCIError(int, char*);

#endif

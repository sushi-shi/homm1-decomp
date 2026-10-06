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
    CD_POSITION_CAPACITY = 15
H1_ENUM_CONST_END(CDPlaybackConstant)

// gConfig.musicVolume/soundVolume levels as the system menu labels them:
// OFF silences the channel, level 1 is full volume and level 10 the quietest
// (gVolumeLevels); FIRST..LAST is the audible range the control panel
// cycles through.
H1_ENUM_CONST_BEGIN(ConfigVolumeLevel)
    SOUND_VOLUME_OFF = 0,
    SOUND_VOLUME_100 = 1,
    SOUND_VOLUME_90 = 2,
    SOUND_VOLUME_80 = 3,
    SOUND_VOLUME_70 = 4,
    SOUND_VOLUME_60 = 5,
    SOUND_VOLUME_50 = 6,
    SOUND_VOLUME_40 = 7,
    SOUND_VOLUME_30 = 8,
    SOUND_VOLUME_20 = 9,
    SOUND_VOLUME_10 = 10,
    SOUND_VOLUME_FIRST = SOUND_VOLUME_100,
    SOUND_VOLUME_LAST = SOUND_VOLUME_10
H1_ENUM_CONST_END(ConfigVolumeLevel)

H1_ENUM_BEGIN(SampleReportQuery)
    SAMPLE_REPORT_VOLUME = 1,
    SAMPLE_REPORT_PLAYING = 4
H1_ENUM_END(SampleReportQuery)

extern i32 CDPlaying;
extern i32 CDPlayOnce;
extern i8 CDTrackMap[];
extern char CDPreviousPosition[][CD_POSITION_CAPACITY];
extern char gMciCommandString[];
extern char gMciReturnString[];
extern u32 gMciError;
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

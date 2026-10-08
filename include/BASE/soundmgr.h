#ifndef HOMM1_BASE_SOUNDMGR_H
#define HOMM1_BASE_SOUNDMGR_H

#include <Domains.h>

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

#endif // HOMM1_BASE_SOUNDMGR_H

#ifndef HOMM1_BASE_SAMPLE_H
#define HOMM1_BASE_SAMPLE_H

#include <BASE/resource.h>

enum SamplePlaybackChannel {
    SAMPLE_PLAYBACK_CHANNEL_MUSIC = 0,
    SAMPLE_PLAYBACK_CHANNEL_GROUP = 2,
    SAMPLE_PLAYBACK_CHANNEL_NONE = 4
};

enum SampleDefaultConstant {
    SAMPLE_VOLUME_FULL = 127,
    SAMPLE_LOOP_ONCE = 1
};

#pragma pack(push, 1)
struct SamplePlaybackData {
    struct _SAMPLE* activeSample;
    i8* data;
    i32 size;
    i32 channelType;
    i32 sampleRate;
    i32 format;
    i32 volume;
    i32 loopCount;
};

class sample : public resource {
public:
    SamplePlaybackData m_playbackData;

    sample(char* name, i32 channelType, i32 volume, i32 loopCount);
    virtual ~sample();
};
#pragma pack(pop)

struct SAMPLE2 {
    class sample* pSample;
    struct _SAMPLE* pMem;
};

#endif

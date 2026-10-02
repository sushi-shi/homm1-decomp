#ifndef HOMM1_BASE_SAMPLE_H
#define HOMM1_BASE_SAMPLE_H

#include <BASE/resource.h>

H1_ENUM_BEGIN(SamplePlaybackChannel)
    SAMPLE_PLAYBACK_CHANNEL_MUSIC = 0,
    SAMPLE_PLAYBACK_CHANNEL_GROUP = 2,
    SAMPLE_PLAYBACK_CHANNEL_NONE = 4
H1_ENUM_END(SamplePlaybackChannel)

#pragma pack(push, 1)
// MemorySample addresses these fields through one sub-object pointer.
struct SamplePlaybackData {
    struct _SAMPLE* activeSample;
    signed char* data;
    long size;
    H1_ENUM_STORAGE(SamplePlaybackChannel, long) channelType;
    long sampleRate;
    long format;
    long volume;
    long loopCount;
};

class sample : public resource {
public:
    SamplePlaybackData m_playbackData;

    sample(char*, long int, long int, long int);
    virtual ~sample();
};
#pragma pack(pop)

#endif // HOMM1_BASE_SAMPLE_H

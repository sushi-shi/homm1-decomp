#ifndef HOMM1_BASE_SAMPLE_H
#define HOMM1_BASE_SAMPLE_H

#include <BASE/resource.h>

H1_ENUM_BEGIN(SamplePlaybackChannel)
    SAMPLE_PLAYBACK_CHANNEL_GROUP = 2
H1_ENUM_END(SamplePlaybackChannel)

#pragma pack(push, 1)
class sample : public resource {
public:
    void *m_activeSample;
    signed char *m_data;
    long m_size;
    H1_ENUM_STORAGE(SamplePlaybackChannel, long) m_channelType;
    long m_sampleRate;
    long m_format;
    long m_volume;
    long m_loopCount;

    sample(char *, long int, long int, long int);
    virtual ~sample();
};
#pragma pack(pop)

#endif // HOMM1_BASE_SAMPLE_H

#ifndef HOMM1_BASE_SAMPLE_H
#define HOMM1_BASE_SAMPLE_H

#include <BASE/resource.h>

enum SampleDefaultConstant {
    SAMPLE_VOLUME_FULL = 127
};

struct SamplePlaybackData {
    i8* data;
    i32 size;
    i32 sampleRate;
    i32 volume;
    i32 sampleFormat;
    i32 stereo;
    b32 repeat;
};

class sample : public resource {
public:
    SamplePlaybackData m_playbackData;

    sample(char* name);
    virtual ~sample();
};

enum SampleLoadConstant {
    SAMPLE_FILENAME_CAPACITY = 32,
    SAMPLE_FORMAT_SUFFIX_LENGTH = 3,
    SAMPLE_LOAD_RATE_11025 = 11025,
    SAMPLE_LOAD_RATE_22050 = 22050,
    SAMPLE_LOAD_RATE_44100 = 44100,
    SAMPLE_LOAD_FORMAT_8_BIT = 0,
    SAMPLE_LOAD_FORMAT_16_BIT = 1,
    SAMPLE_LOAD_MONO = 0,
    SAMPLE_LOAD_STEREO = 1,
    SAMPLE_CHANNEL_COUNT_MONO = 1,
    SAMPLE_CHANNEL_COUNT_STEREO = 2
};

#endif

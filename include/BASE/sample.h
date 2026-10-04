#ifndef HOMM1_BASE_SAMPLE_H
#define HOMM1_BASE_SAMPLE_H

#include <BASE/resource.h>

H1_ENUM_CONST_BEGIN(SampleDefaultConstant)
    SAMPLE_VOLUME_FULL = 127
H1_ENUM_CONST_END(SampleDefaultConstant)

#pragma pack(push, 1)
// Buka's Audiere buffer description; the resource base occupies 0x0e bytes.
struct SamplePlaybackData {
    i8* data;
    i32 size;
    i32 sampleRate;
    i32 volume;
    i32 sampleFormat;
    i32 stereo;
    i32 repeat;
};

class sample : public resource {
public:
    SamplePlaybackData m_playbackData;

    sample(char* name);
    virtual ~sample();
};
#pragma pack(pop)

#endif

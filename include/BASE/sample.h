#ifndef HOMM1_BASE_SAMPLE_H
#define HOMM1_BASE_SAMPLE_H

#include <BASE/resource.h>

#pragma pack(push, 1)
class sample : public resource {
public:
    void *m_activeSample;
    signed char *m_data;
    long m_size;
    long m_volume;
    long m_sampleRate;
    long m_format;
    long m_loopCount;
    long m_channelType;

    sample(char *, long int, long int, long int);
    virtual ~sample();
};
#pragma pack(pop)

#endif // HOMM1_BASE_SAMPLE_H

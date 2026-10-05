#ifndef HOMM1_BASE_AUDIEREBACKEND_H
#define HOMM1_BASE_AUDIEREBACKEND_H

#include <BASE/sample.h>

#include <audiere.h>

// Buka replaces soundManager with free functions. These names describe the
// recovered behavior; the executable has no surviving C++ symbols.
struct AudiereDevice {
    static audiere::AudioDevicePtr device;
};
struct AudiereMusic {
    static audiere::OutputStreamPtr stream;
    static audiere::SampleSourcePtr source;
};

audiere::AudioDevicePtr GetAudioDevice();

struct AudiereSampleNode {
    audiere::OutputStreamPtr stream;
    sample* resource;
    AudiereSampleNode* next;

    AudiereSampleNode(sample* sampleResource, AudiereSampleNode* nextNode) {
        stream = NULL;
        resource = sampleResource;
        next = nextNode;
    }
    inline ~AudiereSampleNode();
};

#endif

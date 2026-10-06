#ifndef HOMM1_BASE_AUDIEREBACKEND_H
#define HOMM1_BASE_AUDIEREBACKEND_H

#include <BASE/sample.h>

#include <audiere.h>

// Buka replaces soundManager with free functions. These names describe the
// recovered behavior; the executable has no surviving C++ symbols.

audiere::AudioDevicePtr GetAudioDevice();

// A playing-sample list node. As a class template its destructor is emitted
// after the RefPtr instances, as in retail (0x00469f80 follows them).
template <class Resource> struct AudiereNode {
    audiere::OutputStreamPtr stream;
    Resource* resource;
    AudiereNode* next;

    AudiereNode(Resource* sampleResource, AudiereNode* nextNode) {
        stream = NULL;
        resource = sampleResource;
        next = nextNode;
    }
    ~AudiereNode() {}
};
typedef AudiereNode<sample> AudiereSampleNode;

struct AudiereMusic {
    static audiere::OutputStreamPtr stream;
    static audiere::SampleSourcePtr source;
};
struct AudiereDevice {
    static audiere::AudioDevicePtr device;
};

#endif

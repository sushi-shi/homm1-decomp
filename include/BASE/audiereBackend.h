#ifndef HOMM1_BASE_AUDIEREBACKEND_H
#define HOMM1_BASE_AUDIEREBACKEND_H

#include <BASE/sample.h>

#include <audiere.h>

audiere::AudioDevicePtr GetAudioDevice();

template<class Resource> struct AudiereNode {
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
    static audiere::OutputStreamPtr channel;
    static audiere::SampleSourcePtr origin;
};
struct AudiereDevice {
    static audiere::AudioDevicePtr driver;
    static int dummy;
};

#endif

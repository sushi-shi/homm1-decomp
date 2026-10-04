// Buka 2003 resource-backed Audiere samples.

#include <match.h>

#include <BASE/audio.h>
#include <BASE/audiereBackend.h>
#include <BASE/resourceManager.h>
#include <BASE/sample.h>
#include <SOURCE/KB.h>
#include <SOURCE/NOOPT.h>

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

// Buka retail VA 0x004cdf58.
static void* gSampleBuffer;
// Buka retail VA 0x004cdf5c.
static int gSampleFrames;
// Buka retail VA 0x004cdf60.
static int gSampleChannels;
// Buka retail VA 0x004cdf64.
static int gSampleRate;
// Buka retail VA 0x004cdf68.
static audiere::SampleFormat gSampleFormat;
// Buka retail VA 0x004ce104.
static AudiereSampleNode* gSamples;
// Buka retail VA 0x004ce108.
static int gSampleSuspensions;

VA(0x004689a0, 0x162)
void CleanupSamples() {
    if (gSamples == NULL)
        return;
    AudiereSampleNode* head = NULL;
    for (;;) {
        if (!gSamples->stream->isPlaying()) {
            head = gSamples->next;
            delete gSamples;
            gSamples = head;
            if (gSamples == NULL)
                return;
        } else {
            break;
        }
    }
    AudiereSampleNode* current = gSamples->next;
    AudiereSampleNode* previous = gSamples;
    while (current != NULL) {
        if (!current->stream->isPlaying()) {
            previous->next = current->next;
            delete current;
            current = previous->next;
        } else {
            previous = current;
            current = current->next;
        }
    }
}

VA(0x00468b02, 0x35)
AudiereSampleNode* FindSampleNode(sample* resource) {
    for (AudiereSampleNode* node = gSamples; node != NULL; node = node->next) {
        if (node->resource == resource)
            return node;
    }
    return NULL;
}

VA(0x00468b37, 0x38d)
void PlaySample(sample* resource) {
    if (!GetAudioDevice())
        return;
    if (SamplesSuspended())
        return;
    AudiereSampleNode* node = FindSampleNode(resource);
    if (node != NULL) {
        if (node->stream->isPlaying())
            return;
        CleanupSamples();
    }
    gSamples = new AudiereSampleNode(resource, gSamples);
    gSampleBuffer = resource->m_playbackData.data;
    gSampleRate = resource->m_playbackData.sampleRate;
    gSampleFrames = resource->m_playbackData.size;
    if (resource->m_playbackData.stereo != 0) {
        gSampleChannels = 2;
        gSampleFrames >>= 1;
    } else {
        gSampleChannels = 1;
    }
    if (resource->m_playbackData.sampleFormat != 0) {
        gSampleFormat = audiere::SF_S16;
        gSampleFrames >>= 1;
    } else {
        gSampleFormat = audiere::SF_U8;
    }
    gSamples->stream =
        GetAudioDevice()
            ->openBuffer(gSampleBuffer, gSampleFrames, gSampleChannels, gSampleRate, gSampleFormat);
    if (!gSamples->stream) {
        AudiereSampleNode* dead = gSamples;
        gSamples = gSamples->next;
        delete dead;
    } else {
        gSamples->stream->setVolume(ScaleSampleVolume(resource->m_playbackData.volume));
        gSamples->stream->setRepeat(resource->m_playbackData.repeat != 0 ? true : false);
        gSamples->stream->play();
    }
    CleanupSamples();
}

VA(0x00468ec4, 0x29)
sample* LoadPlaySample(char* name) {
    sample* resource = gpResourceManager->GetSample(name);
    PlaySample(resource);
    return resource;
}

VA(0x00468eed, 0x71)
void StopSample(sample* resource) {
    if (SamplesSuspended())
        return;
    AudiereSampleNode* node = FindSampleNode(resource);
    if (node != NULL) {
        if (node->stream->isPlaying())
            node->stream->stop();
        CleanupSamples();
    }
}

VA(0x00468f5e, 0x58)
void UpdateSampleVolume(sample* resource) {
    if (SamplesSuspended())
        return;
    AudiereSampleNode* node = FindSampleNode(resource);
    if (node != NULL)
        node->stream->setVolume(ScaleSampleVolume(resource->m_playbackData.volume));
}

VA(0x00468fb6, 0x63)
void WaitSample(sample* resource) {
    if (SamplesSuspended())
        return;
    AudiereSampleNode* node = FindSampleNode(resource);
    if (node != NULL) {
        while (node->stream->isPlaying())
            DelayMilli(10);
        CleanupSamples();
    }
}

VA(0x00469019, 0x77)
void StopAllSamples() {
    if (SamplesSuspended())
        return;
    for (AudiereSampleNode* node = gSamples; node != NULL; node = node->next) {
        if (node->stream->isPlaying())
            node->stream->stop();
    }
    CleanupSamples();
}

VA(0x00469090, 0x61)
void UpdateAllSampleVolumes() {
    if (SamplesSuspended())
        return;
    for (AudiereSampleNode* node = gSamples; node != NULL; node = node->next)
        node->stream->setVolume(ScaleSampleVolume(node->resource->m_playbackData.volume));
}

VA(0x004690f1, 0x12)
void SuspendSamples() {
    ++gSampleSuspensions;
}

VA(0x00469103, 0x12)
void ResumeSamples() {
    --gSampleSuspensions;
}

VA(0x00469115, 0x11)
bool SamplesSuspended() {
    return gSampleSuspensions > 0;
}

inline AudiereSampleNode::~AudiereSampleNode() {}

// Buka 2003 Audiere audio: resource-backed samples, Ogg music playback, and
// the device and volume controls. Retail compiled these as one translation
// unit (contiguous unaligned code, one <string> ctype pair, shared template
// instances); see docs/buka-2003.md. English game text remains in the catalogs.

#include <match.h>

#include <BASE/audiereBackend.h>
#include <BASE/audio.h>
#include <BASE/resourceManager.h>
#include <BASE/sample.h>
#include <SOURCE/KB.h>
#include <SOURCE/NOOPT.h>

#include <stdio.h>

// No retail code reads this; it holds its retail .bss place.
DATA(0x004cdde4)
static int gAudioOldStore;
#define gSampleBuffer gSampleBufferly // spelling fixes .bss order
DATA(0x004cdf58)
static void* gSampleBuffer;
#define gSampleFrames gSampleFrames0 // spelling fixes .bss order
DATA(0x004cdf5c)
static int gSampleFrames;
#define gSampleChannels gSampleChannelsa58 // spelling fixes .bss order
DATA(0x004cdf60)
static int gSampleChannels;
#define gSampleRate gSampleRate7 // spelling fixes .bss order
DATA(0x004cdf64)
static int gSampleRate;
#define gSampleFormat gSampleFormata98 // spelling fixes .bss order
DATA(0x004cdf68)
static audiere::SampleFormat gSampleFormat;
#define gSamples gSamplesa // spelling fixes .bss order
DATA(0x004ce104)
static AudiereSampleNode* gSamples;
#define gSampleSuspensions gSampleSuspensionsa6 // spelling fixes .bss order
DATA(0x004ce108)
static int gSampleSuspensions;

#define nextNode head // frame-slot spelling
VA(0x004689a0, 0x162)
void CleanupSamples() {
    if (gSamples == NULL)
        return;
    AudiereSampleNode* nextNode = NULL;
    for (;;) {
        if (!gSamples->stream->isPlaying()) {
            nextNode = gSamples->next;
            delete gSamples;
            gSamples = nextNode;
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
#undef nextNode

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
    sample* resource = gResourceManager->GetSample(name);
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

DATA(0x004cddec)
audiere::OutputStreamPtr AudiereMusic::channel;
DATA(0x004cdf6c)
audiere::SampleSourcePtr AudiereMusic::origin;
DATA(0x004cdde8)
static int gMusicSuspensions;
// The playing MusicTrack as the backend's integer: -1 for none, and the
// gMusicPositions/gCDTrackMap index.
DATA(0x004a0d70)
static int gCurrentTrack = -1;
DATA(0x004a0d74)
static int gCDTrackMap[100] = {
    2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
    22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, -1, -1, -1, -1, -1, -1, -1,
    35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 50,
};

DATA(0x004cddf0)
static char gMusicFilename[352];
#define gMusicPositions gMusicPositionsaat // spelling fixes .bss order
DATA(0x004cdf74)
static int gMusicPositions[100];
#define gMusicSource gMusicSourcejxy // spelling fixes .bss order
DATA(0x004ce10c)
static int gMusicSource;

VA(0x004692b6, 0x47)
bool ShouldRepeatMusic(H1_ENUM_PARAM(MusicTrack, int) track) {
    if (track < MUSIC_TRACK_TERRAIN_END
        || (track >= MUSIC_TRACK_BATTLE_FIRST && track <= MUSIC_TRACK_BATTLE_LAST)
        || track == MUSIC_TRACK_BATTLE_4 || track == MUSIC_TRACK_CONGRATULATIONS
        || track == MUSIC_TRACK_TAVERN || track == MUSIC_TRACK_MAIN_MENU
        || track == MUSIC_TRACK_AI_TURN
        || (track >= MUSIC_TRACK_TOWN_FIRST && track <= MUSIC_TRACK_TOWN_LAST))
        return true;
    return false;
}

VA(0x004692fd, 0x43b)
void PlayMusic(int track) {
    if (!GetAudioDevice())
        return;
    if (MusicSuspended())
        return;
    if (track == gCurrentTrack) {
        if (AudiereMusic::channel) {
            if (!AudiereMusic::channel->isPlaying())
                AudiereMusic::channel->play();
        }
        return;
    }
    if (track < 0) {
        StopMusic();
        return;
    }
    if (gMusicSource == 0) {
        sprintf(gMusicFilename, "%sHeroes%02d.ogg", gSoundPath, track);
    } else {
        int discTrack = gCDTrackMap[track];
        sprintf(
            gMusicFilename,
            "%s%s%02d-AudioTrack %02d.ogg",
            gRegCDRomPath,
            gTracksPath,
            discTrack,
            discTrack
        );
    }
    audiere::SampleSourcePtr source = audiere::OpenSampleSource(gMusicFilename);
    if (source) {
        audiere::OutputStreamPtr stream = GetAudioDevice()->openStream(source.get());
        if (stream) {
            stream->setVolume(GetMusicVolume());
            stream->setRepeat(ShouldRepeatMusic(track));
            if (gMusicPositions[track] > 0 && stream->isSeekable())
                stream->setPosition(gMusicPositions[track]);
            stream->play();
            if (AudiereMusic::channel) {
                if (gCurrentTrack >= 0) {
                    if (AudiereMusic::channel->isSeekable() && ShouldRepeatMusic(gCurrentTrack))
                        gMusicPositions[gCurrentTrack] = AudiereMusic::channel->getPosition();
                    else
                        gMusicPositions[gCurrentTrack] = 0;
                }
                AudiereMusic::channel->stop();
            }
            AudiereMusic::channel = stream;
            AudiereMusic::origin = source;
            gCurrentTrack = track;
        } else {
            StopMusic();
        }
    } else {
        StopMusic();
    }
}

VA(0x00469738, 0xa)
H1_ENUM_RETURN(MusicTrack, int) GetCurrentTrack() {
    return H1_ENUM_DECODE(MusicTrack, gCurrentTrack);
}

VA(0x00469742, 0x164)
void StopMusic() {
    if (MusicSuspended())
        return;
    if (AudiereMusic::channel) {
        if (gCurrentTrack >= 0) {
            if (AudiereMusic::channel->isSeekable() && ShouldRepeatMusic(gCurrentTrack))
                gMusicPositions[gCurrentTrack] = AudiereMusic::channel->getPosition();
            else
                gMusicPositions[gCurrentTrack] = 0;
        }
        AudiereMusic::channel->stop();
        AudiereMusic::channel = NULL;
    }
    AudiereMusic::origin = NULL;
    gCurrentTrack = -1;
}

VA(0x004698a6, 0x55)
void UpdateMusicVolume() {
    if (MusicSuspended())
        return;
    if (!AudiereMusic::channel)
        return;
    AudiereMusic::channel->setVolume(GetMusicVolume());
}

VA(0x004698fb, 0x128)
void SetMusicSource(int source) {
    if (AudiereMusic::channel) {
        AudiereMusic::channel->stop();
        AudiereMusic::channel = NULL;
    }
    AudiereMusic::origin = NULL;
    for (int track = 0; track <= 99; ++track)
        gMusicPositions[track] = 0;
    gMusicSource = source;
    if (gCurrentTrack >= 0) {
        int oldTrack = gCurrentTrack;
        gCurrentTrack = -1;
        PlayMusic(oldTrack);
    }
}

VA(0x00469a23, 0x36)
bool MusicPlaying() {
    if (!AudiereMusic::channel)
        return false;
    return AudiereMusic::channel->isPlaying();
}

VA(0x00469a59, 0x12)
void SuspendMusic() {
    ++gMusicSuspensions;
}

VA(0x00469a6b, 0x12)
void ResumeMusic() {
    --gMusicSuspensions;
}

VA(0x00469a7d, 0x11)
bool MusicSuspended() {
    return gMusicSuspensions > 0;
}

DATA(0x004cdf50)
audiere::AudioDevicePtr AudiereDevice::driver;
DATA(0x004cdf54)
int AudiereDevice::dummy;
DATA(0x004a0f04)
static float gEffectsVolume = 1.0f;
DATA(0x004a0f08)
static float gMusicVolume = 1.0f;
DATA(0x004a0f0c)
static float gVolumeLevels[11] =
    {0.0f, 1.0f, 0.8f, 0.65f, 0.5f, 0.4f, 0.3f, 0.2f, 0.15f, 0.1f, 0.05f};

VA(0x00469b56, 0xf)
float VolumeLevel(int level) {
    return gVolumeLevels[level];
}

VA(0x00469b65, 0x70)
audiere::AudioDevicePtr GetAudioDevice() {
    return AudiereDevice::driver;
}

VA(0x00469bd5, 0x132)
bool InitAudio() {
    audiere::AudioDevice* device = audiere::OpenDevice("winmm", NULL);
    if (device) {
        AudiereDevice::driver = device;
    } else {
        AudiereDevice::driver = audiere::OpenDevice("null", NULL);
        if (!AudiereDevice::driver)
            return false;
        // Retail releases even a successful fallback device (RVA 0x69caf).
        AudiereDevice::driver = NULL;
        return true;
    }
    return true;
}

VA(0x00469d07, 0x5c)
void ShutdownAudio() {
    StopAllAudio();
    AudiereDevice::driver = NULL;
}

VA(0x00469d63, 0xb)
float GetEffectsVolume() {
    return gEffectsVolume;
}

VA(0x00469d6e, 0x15)
float ScaleSampleVolume(int volume) {
    return GetEffectsVolume() * (static_cast<float>(volume)) / 127.0f;
}

VA(0x00469d83, 0xb)
float GetMusicVolume() {
    return gMusicVolume;
}

VA(0x00469d8e, 0xf)
void StopAllAudio() {
    StopAllSamples();
    StopMusic();
}

VA(0x00469d9d, 0x1c)
void SetEffectsVolume(int level) {
    gEffectsVolume = VolumeLevel(level);
    UpdateAllSampleVolumes();
}

VA(0x00469db9, 0x1c)
void SetMusicVolume(int level) {
    gMusicVolume = VolumeLevel(level);
    UpdateMusicVolume();
}

VA(0x00469dd5, 0x1d)
void SetVolumes(int effects, int music) {
    SetEffectsVolume(effects);
    SetMusicVolume(music);
}

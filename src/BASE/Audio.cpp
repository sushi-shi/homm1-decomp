#include <H1/Ints.h>

#include <mss.h>

#include <BASE/audiereBackend.h>
#include <BASE/audio.h>
#include <BASE/resourceManager.h>
#include <BASE/sample.h>
#include <SOURCE/KB.h>
#include <SOURCE/NOOPT.h>
#include <SOURCE/smackManager.h>

#include <limits.h>
#include <stdio.h>

#ifndef HOMM1_EDITOR
static i32 gAudioOldStore;
#endif
static void* gSampleBuffer;
static i32 gSampleFrames;
static i32 gSampleChannels;
static i32 gSampleRate;
static audiere::SampleFormat gSampleFormat;
static AudiereSampleNode* gSamples;
static i32 gSampleSuspensions;

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

AudiereSampleNode* FindSampleNode(sample* resource) {
    for (AudiereSampleNode* node = gSamples; node != NULL; node = node->next) {
        if (node->resource == resource)
            return node;
    }
    return NULL;
}

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
    if (resource->m_playbackData.stereo != SAMPLE_LOAD_MONO) {
        gSampleChannels = SAMPLE_CHANNEL_COUNT_STEREO;
        gSampleFrames >>= 1;
    } else {
        gSampleChannels = SAMPLE_CHANNEL_COUNT_MONO;
    }
    if (resource->m_playbackData.sampleFormat != SAMPLE_LOAD_FORMAT_8_BIT) {
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
        gSamples->stream->setRepeat(resource->m_playbackData.repeat != false ? true : false);
        gSamples->stream->play();
    }
    CleanupSamples();
}

sample* LoadPlaySample(char* name) {
    sample* resource = gResourceManager->GetSample(name);
    PlaySample(resource);
    return resource;
}

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

void UpdateSampleVolume(sample* resource) {
    if (SamplesSuspended())
        return;
    AudiereSampleNode* node = FindSampleNode(resource);
    if (node != NULL)
        node->stream->setVolume(ScaleSampleVolume(resource->m_playbackData.volume));
}

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

void StopAllSamples() {
    if (SamplesSuspended())
        return;
    for (AudiereSampleNode* node = gSamples; node != NULL; node = node->next) {
        if (node->stream->isPlaying())
            node->stream->stop();
    }
    CleanupSamples();
}

void UpdateAllSampleVolumes() {
    if (SamplesSuspended())
        return;
    for (AudiereSampleNode* node = gSamples; node != NULL; node = node->next)
        node->stream->setVolume(ScaleSampleVolume(node->resource->m_playbackData.volume));
}

void SuspendSamples() {
    ++gSampleSuspensions;
}

void ResumeSamples() {
    --gSampleSuspensions;
}

bool SamplesSuspended() {
    return gSampleSuspensions > 0;
}

audiere::OutputStreamPtr AudiereMusic::channel;
audiere::SampleSourcePtr AudiereMusic::origin;
static i32 gMusicSuspensions;
static i32 gCurrentTrack = -1;
static i32 gCDTrackMap[AUDIO_TRACK_SLOT_COUNT] = {
    2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
    22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, -1, -1, -1, -1, -1, -1, -1,
    35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 50,
};

static char gMusicFilename[352];
static i32 gMusicPositions[AUDIO_TRACK_SLOT_COUNT];
static i32 gMusicSource;

bool ShouldRepeatMusic(i32 track) {
    if (track < MUSIC_TRACK_TERRAIN_END
        || (track >= MUSIC_TRACK_BATTLE_FIRST && track <= MUSIC_TRACK_BATTLE_LAST)
        || track == MUSIC_TRACK_BATTLE_4 || track == MUSIC_TRACK_CONGRATULATIONS
        || track == MUSIC_TRACK_TAVERN || track == MUSIC_TRACK_MAIN_MENU
        || track == MUSIC_TRACK_AI_TURN
        || (track >= MUSIC_TRACK_TOWN_FIRST && track <= MUSIC_TRACK_TOWN_LAST))
        return true;
    return false;
}

void PlayMusic(i32 track) {
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
    if (gMusicSource == SOUND_MUSIC_SOURCE_DIGITAL) {
        sprintf(gMusicFilename, "%sHeroes%02d.ogg", gSoundPath, track);
    } else {
        i32 discTrack = gCDTrackMap[track];
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

i32 GetCurrentTrack() {
    return gCurrentTrack;
}

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
    gCurrentTrack = MUSIC_TRACK_NONE;
}

void UpdateMusicVolume() {
    if (MusicSuspended())
        return;
    if (!AudiereMusic::channel)
        return;
    AudiereMusic::channel->setVolume(GetMusicVolume());
}

void SetMusicSource(i32 source) {
    if (AudiereMusic::channel) {
        AudiereMusic::channel->stop();
        AudiereMusic::channel = NULL;
    }
    AudiereMusic::origin = NULL;
    for (i32 track = 0; track <= AUDIO_TRACK_SLOT_LAST; ++track)
        gMusicPositions[track] = 0;
    gMusicSource = source;
    if (gCurrentTrack >= 0) {
        i32 oldTrack = gCurrentTrack;
        gCurrentTrack = MUSIC_TRACK_NONE;
        PlayMusic(oldTrack);
    }
}

bool MusicPlaying() {
    if (!AudiereMusic::channel)
        return false;
    return AudiereMusic::channel->isPlaying();
}

void SuspendMusic() {
    ++gMusicSuspensions;
}

void ResumeMusic() {
    --gMusicSuspensions;
}

bool MusicSuspended() {
    return gMusicSuspensions > 0;
}

audiere::AudioDevicePtr AudiereDevice::driver;
i32 AudiereDevice::dummy;
static float gEffectsVolume = 1.0f;
static float gMusicVolume = 1.0f;
static float gVolumeLevels[11] =
    {0.0f, 1.0f, 0.8f, 0.65f, 0.5f, 0.4f, 0.3f, 0.2f, 0.15f, 0.1f, 0.05f};

float VolumeLevel(i32 level) {
    return gVolumeLevels[level];
}

audiere::AudioDevicePtr GetAudioDevice() {
    return AudiereDevice::driver;
}

bool InitAudio() {
    audiere::AudioDevice* device = audiere::OpenDevice("winmm", NULL);
    if (device) {
        AudiereDevice::driver = device;
    } else {
        AudiereDevice::driver = audiere::OpenDevice("null", NULL);
        if (!AudiereDevice::driver)
            return false;
        AudiereDevice::driver = NULL;
        return true;
    }
    return true;
}

void ShutdownAudio() {
    StopAllAudio();
    AudiereDevice::driver = NULL;
}

float GetEffectsVolume() {
    return gEffectsVolume;
}

float ScaleSampleVolume(i32 volume) {
    return GetEffectsVolume() * (static_cast<float>(volume)) / 127.0f;
}

float GetMusicVolume() {
    return gMusicVolume;
}

void StopAllAudio() {
    StopAllSamples();
    StopMusic();
}

void SetEffectsVolume(i32 level) {
    gEffectsVolume = VolumeLevel(level);
    UpdateAllSampleVolumes();
}

void SetMusicVolume(i32 level) {
    gMusicVolume = VolumeLevel(level);
    UpdateMusicVolume();
}

void SetVolumes(i32 effects, i32 music) {
    SetEffectsVolume(effects);
    SetMusicVolume(music);
}

#ifndef HOMM1_EDITOR

// Movie sound goes through Miles, which Smacker drives directly. The editor
// plays no movies.

#pragma pack(push, 1)
struct SmackSoundFormat {
    DWORD format;
    WORD channels;
    DWORD samplesPerSecond;
    WORD bitsPerSample;
};
#pragma pack(pop)

static SmackSoundFormat gSmackSoundFormats[SMACK_SOUND_FORMAT_COUNT] = {
    {WAVE_FORMAT_4S16, 2, 44100, 16},
    {WAVE_FORMAT_4S08, 2, 44100, 8},
    {WAVE_FORMAT_4M16, 1, 44100, 16},
    {WAVE_FORMAT_4M08, 1, 44100, 8},
    {WAVE_FORMAT_2S16, 2, 22050, 16},
    {WAVE_FORMAT_2S08, 2, 22050, 8},
    {WAVE_FORMAT_2M16, 1, 22050, 16},
    {WAVE_FORMAT_2M08, 1, 22050, 8},
    {WAVE_FORMAT_1S16, 2, 11025, 16},
    {WAVE_FORMAT_1S08, 2, 11025, 8},
    {WAVE_FORMAT_1M16, 1, 11025, 16},
    {WAVE_FORMAT_1M08, 1, 11025, 8}
};
static WAVEOUTCAPS gSmackWaveCaps;
static SmackSoundFormat gSmackAudioFormat;
static PCMWAVEFORMAT gSmackPcmFormat;
static HDIGDRIVER gSmackDigDriver;

void InitSmackSound() {
    if (gSmackDigDriver)
        return;
    if (!waveOutGetNumDevs())
        return;
    if (waveOutGetDevCaps(0, &gSmackWaveCaps, sizeof(gSmackWaveCaps)))
        return;
    gSmackAudioFormat.format = 0;
    for (u32 i = 0; i < SMACK_SOUND_FORMAT_COUNT; ++i) {
        if (gSmackWaveCaps.dwFormats & gSmackSoundFormats[i].format) {
            gSmackAudioFormat.format = gSmackSoundFormats[i].format;
            gSmackAudioFormat.channels = gSmackSoundFormats[i].channels;
            gSmackAudioFormat.samplesPerSecond = gSmackSoundFormats[i].samplesPerSecond;
            gSmackAudioFormat.bitsPerSample = gSmackSoundFormats[i].bitsPerSample;
            break;
        }
    }
    if (!gSmackAudioFormat.format) {
        gSmackAudioFormat.channels = SMACK_FALLBACK_CHANNELS;
        gSmackAudioFormat.samplesPerSecond = SMACK_FALLBACK_SAMPLE_RATE;
        gSmackAudioFormat.bitsPerSample = SMACK_FALLBACK_BITS_PER_SAMPLE;
    }
    AIL_startup();
    gSmackPcmFormat.wf.wFormatTag = WAVE_FORMAT_PCM;
    gSmackPcmFormat.wf.nChannels = gSmackAudioFormat.channels;
    gSmackPcmFormat.wf.nSamplesPerSec = gSmackAudioFormat.samplesPerSecond;
    gSmackPcmFormat.wf.nAvgBytesPerSec = gSmackAudioFormat.samplesPerSecond
                                         * (gSmackAudioFormat.bitsPerSample / CHAR_BIT)
                                         * gSmackAudioFormat.channels;
    gSmackPcmFormat.wf.nBlockAlign =
        (gSmackAudioFormat.bitsPerSample / CHAR_BIT) * gSmackAudioFormat.channels;
    gSmackPcmFormat.wBitsPerSample = gSmackAudioFormat.bitsPerSample;
    if (AIL_waveOutOpen(&gSmackDigDriver, NULL, 0, &gSmackPcmFormat.wf))
        gSmackDigDriver = NULL;
}

void ShutdownSmackSound() {
    if (gSmackDigDriver) {
        AIL_waveOutClose(gSmackDigDriver);
        gSmackDigDriver = NULL;
        AIL_shutdown();
    }
}

i32 SmackSoundReady() {
    return gSmackDigDriver != NULL;
}

void UseSmackSound(i32 volume) {
    AIL_set_digital_master_volume(gSmackDigDriver, volume);
    SmackSoundUseMSS(gSmackDigDriver);
}

#endif
